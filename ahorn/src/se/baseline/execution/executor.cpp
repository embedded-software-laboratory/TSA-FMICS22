#include "se/baseline/execution/executor.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::baseline;

Executor::Executor(Solver &solver)
    : _solver(&solver), _evaluator(std::make_unique<Evaluator>(*_solver)),
      _encoder(std::make_unique<Encoder>(*_solver)), _flattened_name_to_version(std::map<std::string, unsigned int>()),
      _whole_program_inputs(std::set<std::string>()), _context(nullptr), _forked_context(boost::none) {}

bool Executor::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::execute(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("Baseline");
    SPDLOG_LOGGER_TRACE(logger, "\n{}", *context);

    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();
    State &state = context->getState();
    const Vertex &vertex = state.getVertex();

    std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts;
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramEntryVertex(cfg, vertex, state);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockEntryVertex(cfg, vertex, state);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    succeeding_contexts.first = handleFunctionEntryVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered during execution.");
            }
            break;
        }
        case Vertex::Type::REGULAR: {
            succeeding_contexts = handleRegularVertex(vertex, std::move(context));
            break;
        }
        case Vertex::Type::EXIT: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    succeeding_contexts.first = handleProgramExitVertex(std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    succeeding_contexts.first = handleFunctionBlockExitVertex(std::move(context));
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    succeeding_contexts.first = handleFunctionExitVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered during execution.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected vertex type encountered during execution.");
    }

    return succeeding_contexts;
}

void Executor::initialize(const Cfg &cfg) {
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _flattened_name_to_version.emplace(std::move(flattened_name), 0);
    }
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _whole_program_inputs.emplace(std::move(flattened_name));
    }
}

void Executor::handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

std::unique_ptr<Context> Executor::handleFunctionEntryVertex() {
    throw std::logic_error("Not implemented yet.");
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context) {
    _context = std::move(context);
    _forked_context = boost::none;

    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);

    std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts;
    succeeding_contexts.first = std::move(_context);
    if (_forked_context.has_value()) {
        succeeding_contexts.second = std::move(*_forked_context);
    }
    return succeeding_contexts;
}

std::unique_ptr<Context> Executor::handleProgramExitVertex(std::unique_ptr<Context> context) {
    unsigned int cycle = context->getCycle();
    unsigned int next_cycle = cycle + 1;
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    State &state = context->getState();
    state.setVertex(next_vertex);
    context->setCycle(next_cycle);

    // prepare "initial" valuations for the next cycle, relate "local" variables with each other and keep
    // "whole-program" input variables unconstrained
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _flattened_name_to_version.at(flattened_name) = 0;
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(next_cycle);

        // distinguish between "whole-program" input variables and local/output variables
        if (isWholeProgramInput(flattened_name)) {
            z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, it->getDataType());
            state.setConcreteValuation(contextualized_name, _solver->makeRandomValue(it->getDataType()));
            state.setSymbolicValuation(contextualized_name, symbolic_valuation);
        } else {
            unsigned int highest_version = state.getHighestVersion(flattened_name, cycle);
            z3::expr concrete_valuation = state.getConcreteValuation(
                    flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle));
            state.setConcreteValuation(contextualized_name, concrete_valuation);
            state.setSymbolicValuation(contextualized_name, concrete_valuation);
        }
    }

    return context;
}

std::unique_ptr<Context> Executor::handleFunctionBlockExitVertex(std::unique_ptr<Context> context) {
    const Frame &callee_frame = context->getFrame();
    unsigned int next_label = callee_frame.getReturnLabel();
    context->popFrame();
    State &state = context->getState();
    const Frame &caller_frame = context->getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);
    return context;
}

std::unique_ptr<Context> Executor::handleFunctionExitVertex() {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // encode rhs of the assignment
    z3::expr encoded_expression = _encoder->encode(expression, *_context);

    // evaluate rhs of the assignment (before version increment)
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);

    // encode lhs of the assignment
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _flattened_name_to_version.at(flattened_name) + 1;
    _flattened_name_to_version.at(flattened_name) = version;

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    // update concrete and symbolic valuations
    state.setConcreteValuation(contextualized_name, evaluated_expression);
    state.setSymbolicValuation(contextualized_name, encoded_expression);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::CallInstruction &instruction) {
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
    std::shared_ptr<Frame> callee_frame = std::make_shared<Frame>(callee, scope, return_label);

    _context->pushFrame(callee_frame);

    const Vertex &next_vertex = callee.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // evaluate condition concretely
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);
    assert(evaluated_expression.is_bool() && (evaluated_expression.is_true() || evaluated_expression.is_false()));

    // encode condition symbolically
    z3::expr encoded_expression = _encoder->encode(expression, *_context);

    // determine control-flow
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    State &state = _context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &true_edge = cfg.getTrueEdge(label);
    unsigned int next_positive_label = true_edge.getTargetLabel();
    const Vertex &next_positive_vertex = cfg.getVertex(next_positive_label);
    const Edge &false_edge = cfg.getFalseEdge(label);
    unsigned int next_negative_label = false_edge.getTargetLabel();
    const Vertex &next_negative_vertex = cfg.getVertex(next_negative_label);
    const std::vector<z3::expr> &path_constraint = state.getPathConstraint();

    if (evaluated_expression.is_true()) {
        tryFork(state, path_constraint, !encoded_expression, next_negative_vertex);
        state.pushPathConstraint(encoded_expression);
        state.setVertex(cfg.getVertex(next_positive_label));
    } else {
        tryFork(state, path_constraint, encoded_expression, next_positive_vertex);
        state.pushPathConstraint(!encoded_expression);
        state.setVertex(cfg.getVertex(next_negative_label));
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
    // encode lhs of the assignment
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _flattened_name_to_version.at(flattened_name) + 1;
    _flattened_name_to_version.at(flattened_name) = version;

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    std::shared_ptr<ir::Variable> variable = nullptr;
    if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
        variable = variable_access->getVariable();
    } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
        variable = field_access->getVariableAccess().getVariable();
    } else {
        throw std::runtime_error("Unexpected variable reference type encountered.");
    }
    assert(variable != nullptr);
    const auto &data_type = variable->getDataType();
    z3::expr expression = _solver->makeConstant(contextualized_name, data_type);

    // update concrete valuation to a random, type-specific value and symbolic valuation to an unconstrained
    // uninterpreted constant
    state.setConcreteValuation(contextualized_name, _solver->makeRandomValue(data_type));
    state.setSymbolicValuation(contextualized_name, expression);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::tryFork(const State &state, const std::vector<z3::expr> &path_constraint, const z3::expr &expression,
                       const Vertex &vertex) {
    auto logger = spdlog::get("Baseline");
    std::stringstream str;
    for (auto it = path_constraint.begin(); it != path_constraint.end(); ++it) {
        str << (*it).to_string();
        if (std::next(it) != path_constraint.end()) {
            str << ", ";
        }
    }
    SPDLOG_LOGGER_TRACE(logger, "Trying to fork on expression: {} under path constraint: [{}]", expression.to_string(),
                        str.str());
    // XXX check whether expression contains either a "whole-program" input or an "unconstrained" uninterpreted
    // XXX constant, e.g., resulting from a havoc instruction
    if (containsUnconstrainedUninterpretedConstant(state, expression)) {
        z3::expr_vector expressions(_solver->getContext());
        for (const z3::expr &constraint : path_constraint) {
            expressions.push_back(constraint);
        }
        expressions.push_back(expression);
        // add hard constraints, i.e., evaluate path constraint and expression under current valuations
        for (const auto &symbolic_valuation : state.getSymbolicValuations()) {
            if (symbolic_valuation.second.is_bool()) {
                expressions.push_back(_solver->makeBooleanConstant(symbolic_valuation.first) ==
                                      symbolic_valuation.second);
            } else if (symbolic_valuation.second.is_int()) {
                expressions.push_back(_solver->makeIntegerConstant(symbolic_valuation.first) ==
                                      symbolic_valuation.second);
            } else {
                throw std::runtime_error("Unexpected z3::sort encountered.");
            }
        }
        std::pair<z3::check_result, boost::optional<z3::model>> result = _solver->check(expressions);
        switch (result.first) {
            case z3::unsat: {
                assert(!result.second.has_value());
                SPDLOG_LOGGER_TRACE(logger, "Expression under path constraint is unsatisfiable, no fork of execution "
                                            "context.");
                break;
            }
            case z3::sat: {
                // XXX fork
                SPDLOG_LOGGER_TRACE(logger, "Expression under path constraint is satisfiable, fork of execution "
                                            "context.");
                z3::model &model = *result.second;
                _forked_context = _context->fork(*_solver, vertex, model, expression);
                break;
            }
            case z3::unknown: {
                // XXX fall-through
            }
            default:
                throw std::runtime_error("Unexpected z3::check_result encountered.");
        }
    } else {
        SPDLOG_LOGGER_TRACE(logger, "Expression does not contain any unconstrained uninterpreted constants, no fork of "
                                    "execution context possible.");
    }
}

bool Executor::containsUnconstrainedUninterpretedConstant(const State &state, const z3::expr &expression) const {
    std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
    if (uninterpreted_constants.empty()) {
        assert(expression.is_true() || expression.is_false() || expression.is_numeral());
        return false;
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        z3::expr nested_expression = state.getSymbolicValuation(contextualized_name);
        std::vector<z3::expr> nested_uninterpreted_constants = _solver->getUninterpretedConstants(nested_expression);
        if (nested_uninterpreted_constants.empty()) {
            assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
            return false;
        } else if (nested_uninterpreted_constants.size() == 1) {
            std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
            // XXX check for self-reference,checking for whole-program input is not enough, because it could have been
            // XXX assigned or we have a havoc'ed variable
            if (contextualized_name == nested_contextualized_name) {
                return true;
            } else {
                return containsUnconstrainedUninterpretedConstant(state, nested_expression);
            }
        } else {
            return containsUnconstrainedUninterpretedConstant(state, nested_expression);
        }
    } else {
        bool contains_unconstrained_constant = false;
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            if (containsUnconstrainedUninterpretedConstant(state, uninterpreted_constant)) {
                contains_unconstrained_constant = true;
                break;
            } else {
                std::string contextualized_name = uninterpreted_constant.decl().name().str();
                z3::expr nested_expression = state.getSymbolicValuation(contextualized_name);
                if (containsUnconstrainedUninterpretedConstant(state, nested_expression)) {
                    contains_unconstrained_constant = true;
                    break;
                }
            }
        }
        return contains_unconstrained_constant;
    }
}
