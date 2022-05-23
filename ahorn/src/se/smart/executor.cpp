#include "se/smart/executor.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/if_instruction.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <stdexcept>

using namespace se::smart;

Executor::Executor(Manager &manager)
    : _manager(&manager), _backtrack(true), _evaluator(std::make_unique<Evaluator>(manager)),
      _encoder(std::make_unique<Encoder>(manager)), _context(nullptr) {}

std::unique_ptr<Context> Executor::execute(std::unique_ptr<Context> context, const std::stack<unsigned int> &stack) {
    auto logger = spdlog::get("SMART");
    SPDLOG_LOGGER_TRACE(logger, "Executing: {}", *context);

    const State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();

    std::unique_ptr<Context> succeeding_context = nullptr;
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    succeeding_context = handleProgramEntry(cfg, vertex, std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    succeeding_context = handleFunctionBlockEntry(cfg, vertex, std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    throw std::logic_error("Not implemented yet.");
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        }
        case Vertex::Type::REGULAR: {
            succeeding_context = handleRegularVertex(vertex, std::move(context));
            break;
        }
        case Vertex::Type::EXIT: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    succeeding_context = handleProgramExit(cfg, vertex, std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    succeeding_context = handleFunctionBlockExit(cfg, vertex, std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    throw std::logic_error("Not implemented yet.");
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected vertex type encountered.");
    }
    assert(succeeding_context != nullptr);
    return succeeding_context;
}

std::unique_ptr<Context> Executor::handleProgramEntry(const Cfg &cfg, const Vertex &vertex,
                                                      std::unique_ptr<Context> context) {
    unsigned int label = vertex.getLabel();
    State &state = context->getState();
    const Frame &frame = context->getFrame();

    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    return context;
}

std::unique_ptr<Context> Executor::handleFunctionBlockEntry(const Cfg &cfg, const Vertex &vertex,
                                                            std::unique_ptr<Context> context) {
    unsigned int label = vertex.getLabel();
    State &state = context->getState();
    const Frame &frame = context->getFrame();

    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    return context;
}

std::unique_ptr<Context> Executor::handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context) {
    _context = std::move(context);
    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);
    return std::move(_context);
}

std::unique_ptr<Context> Executor::handleFunctionBlockExit(const Cfg &cfg, const Vertex &vertex,
                                                           std::unique_ptr<Context> context) {
    if (_backtrack) {
        // TODO (19.02.2022): Stop the search in f and generate a summary for the current path
        std::unique_ptr<Context> succeeding_context = solvePathConstraint(std::move(context));
        State &state = succeeding_context->getState();
        if (cfg.getExitLabel() == state.getVertex().getLabel()) {
            // stop backtracking and resume execution in calling context
            const Frame &callee_frame = succeeding_context->getFrame();
            unsigned int next_label = callee_frame.getReturnLabel();
            succeeding_context->popFrame();
            const Frame &caller_frame = succeeding_context->getFrame();
            const Cfg &caller_cfg = caller_frame.getCfg();
            const Vertex &next_vertex = caller_cfg.getVertex(next_label);
            state.setVertex(next_vertex);
        } else {
            // do nothing, backtrack
        }
        return succeeding_context;
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

std::unique_ptr<Context> Executor::handleProgramExit(const Cfg &cfg, const Vertex &vertex,
                                                     std::unique_ptr<Context> context) {
    auto logger = spdlog::get("SMART");

    unsigned int cycle = context->getCycle();
    unsigned int next_cycle = cycle + 1;
    const Frame &frame = context->getFrame();
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    State &state = context->getState();
    state.setVertex(next_vertex);
    context->setCycle(next_cycle);
    context->resetExecutedConditionals();

    for (auto flattened_variable = cfg.flattenedInterfaceBegin(); flattened_variable != cfg.flattenedInterfaceEnd();
         ++flattened_variable) {
        std::string flattened_name = flattened_variable->getFullyQualifiedName();
        context->setVersion(flattened_name, 0);
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(next_cycle);

        z3::expr concrete_valuation = flattened_variable->hasInitialization()
                                              ? _manager->makeValue(flattened_variable->getInitialization())
                                              : _manager->makeDefaultValue(flattened_variable->getDataType());

        state.setConcreteValuation(0, contextualized_name, concrete_valuation);

        // distinguish between "whole-program" input variables and local/output variables
        if (context->isWholeProgramInput(flattened_name)) {
            z3::expr symbolic_valuation =
                    _manager->makeConstant(contextualized_name, flattened_variable->getDataType());
            state.setSymbolicValuation(0, contextualized_name, symbolic_valuation);
        } else {
            state.setSymbolicValuation(0, contextualized_name, concrete_valuation);
        }
    }

    SPDLOG_LOGGER_TRACE(logger, "Preparation of context for next cycle: {}", *context);

    return context;
}

std::unique_ptr<Context> Executor::solvePathConstraint(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("SMART");

    const State &state = context->getState();
    // number of "globally" executed conditionals
    int k = context->getExecutedConditionals();
    const Frame &frame = context->getFrame();
    // number of executed conditionals before executing the frame
    int k_f = frame.getExecutedConditionals();
    std::vector<unsigned int> &stack = context->getStack();

    // XXX copy path constraint
    std::vector<z3::expr> path_constraint = state.getPathConstraint();
    std::stringstream str;
    for (auto it = path_constraint.begin(); it != path_constraint.end(); ++it) {
        str << it->to_string();
        if (std::next(it) == path_constraint.end()) {
            str << ", ";
        }
    }
    SPDLOG_LOGGER_TRACE(logger, "Trying to solve path constraint: [{}]", str.str());

    if (k > 0) {
        int j = k - 1;
        while (j >= k_f) {
            if (stack.at(j) == 0) {
                path_constraint.at(j) = (!path_constraint.at(j)).simplify();
                /*SPDLOG_LOGGER_TRACE(
                        logger, "Trying to backtrack at j = {} with path constraint: [{}]", j,
                        fmt::join(std::vector<z3::expr>(path_constraint.begin(), path_constraint.begin() + j + 1),
                                  ", "));*/
                z3::expr_vector expressions(_manager->getZ3Context());
                const auto &symbolic_valuations = state.getSymbolicValuations();
                for (int i = 0; i <= j; ++i) {
                    expressions.push_back(path_constraint.at(i));
                    for (const auto &symbolic_valuation : symbolic_valuations.at(i)) {
                        z3::sort sort = symbolic_valuation.second.get_sort();
                        if (sort.is_bool()) {
                            expressions.push_back(_manager->makeBooleanConstant(symbolic_valuation.first) ==
                                                  symbolic_valuation.second);
                        } else if (sort.is_int()) {
                            expressions.push_back(_manager->makeIntegerConstant(symbolic_valuation.first) ==
                                                  symbolic_valuation.second);
                        } else {
                            throw std::runtime_error("Unexpected z3::sort encountered.");
                        }
                    }
                }
                std::pair<z3::check_result, boost::optional<z3::model>> result = _manager->check(expressions);
                if (result.first == z3::check_result::sat) {
                    assert(result.second.has_value());
                    z3::model model = *result.second;
                    SPDLOG_LOGGER_TRACE(logger, "Found satisfying input valuation, resume search.");
                    stack.at(j) = 1;
                    context->backtrack(j, model,
                                       std::vector<z3::expr>(path_constraint.begin(), path_constraint.begin() + j));
                    return context;
                } else {
                    SPDLOG_LOGGER_TRACE(logger, "No satisfying valuation found, backtrack.");
                    // backtrack
                    j = j - 1;
                }
            } else {
                // backtrack
                j = j - 1;
            }
        }
    }

    if (k_f > 0) {
        throw std::logic_error("Not implemented yet.");
    }

    SPDLOG_LOGGER_TRACE(logger, "Stopping directed search within {}", frame.getScope());
    return context;
}

void Executor::visit(const ir::AssignmentInstruction &instruction) {
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();

    const ir::Expression &expression = instruction.getExpression();

    // evaluate rhs of the assignment (before version increment)
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);

    // encode rhs of the assignment
    z3::expr encoded_expression = _encoder->encode(expression, *_context);

    // encode lhs of the assignment
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _context->getVersion(flattened_name) + 1;
    _context->setVersion(flattened_name, version);

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    // update concrete and symbolic valuations
    unsigned int k = _context->getExecutedConditionals();
    state.setConcreteValuation(k, contextualized_name, evaluated_expression);
    state.setSymbolicValuation(k, contextualized_name, encoded_expression);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::CallInstruction &instruction) {
    if (_backtrack) {
        if (false) {
            // TODO (19.02.2022): Find applicable summary
        } else {
            const Frame &frame = _context->getFrame();
            const Cfg &caller = frame.getCfg();

            State &state = _context->getState();
            const Vertex &vertex = state.getVertex();

            unsigned int label = vertex.getLabel();
            const Edge &intraprocedural_edge = caller.getIntraproceduralCallToReturnEdge(label);
            unsigned int return_label = intraprocedural_edge.getTargetLabel();
            const Edge &interprocedural_edge = caller.getInterproceduralCallEdge(label);
            unsigned int next_label = interprocedural_edge.getTargetLabel();

            const ir::VariableAccess &variable_access = instruction.getVariableAccess();
            std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
            const ir::DataType &data_type = variable->getDataType();
            assert(data_type.getKind() == ir::DataType::Kind::DERIVED_TYPE);
            std::string type_representative_name = data_type.getFullyQualifiedName();
            const Cfg &callee = caller.getCfg(type_representative_name);

            std::string scope = frame.getScope() + "." + variable->getName();
            unsigned int executed_conditionals = _context->getExecutedConditionals();
            std::shared_ptr<Frame> callee_frame =
                    std::make_shared<Frame>(callee, scope, return_label, executed_conditionals);

            _context->pushFrame(callee_frame);

            const Vertex &next_vertex = callee.getVertex(next_label);
            state.setVertex(next_vertex);
        }
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

void Executor::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // evaluate condition concretely
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);
    assert(evaluated_expression.is_bool() && (evaluated_expression.is_true() || evaluated_expression.is_false()));

    // encode condition symbolically
    z3::expr encoded_expression = _encoder->encode(expression, *_context);

    if (_backtrack) {
        const Frame &frame = _context->getFrame();
        const Cfg &cfg = frame.getCfg();
        State &state = _context->getState();
        const Vertex &vertex = state.getVertex();
        unsigned int label = vertex.getLabel();

        if (evaluated_expression.is_true()) {
            state.pushPathConstraint(encoded_expression);
            const Edge &true_edge = cfg.getTrueEdge(label);
            unsigned int next_positive_label = true_edge.getTargetLabel();
            state.setVertex(cfg.getVertex(next_positive_label));
        } else {
            state.pushPathConstraint(!encoded_expression);
            const Edge &false_edge = cfg.getFalseEdge(label);
            unsigned int next_negative_label = false_edge.getTargetLabel();
            state.setVertex(cfg.getVertex(next_negative_label));
        }

        unsigned int k = _context->getExecutedConditionals();
        std::vector<unsigned int> &stack = _context->getStack();
        if (k == stack.size()) {
            stack.push_back(0);
        }
        _context->setExecutedConditionals(k + 1, label);
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

void Executor::visit(const ir::SequenceInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::HavocInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}
