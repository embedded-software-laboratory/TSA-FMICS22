#include "se/oa/execution/executor.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::oa;

Executor::Executor(Solver &solver)
    : _solver(&solver), _encoder(std::make_unique<Encoder>(*_solver)), _flattened_name_to_version(), _context(nullptr),
      _forked_contexts(boost::none) {}

unsigned int Executor::getVersion(const std::string &flattened_name) const {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    return _flattened_name_to_version.at(flattened_name);
}

void Executor::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::execute(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("Oa");
    SPDLOG_LOGGER_TRACE(logger, "Executing context: \n{}", *context);

    State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();

    SPDLOG_LOGGER_INFO(logger, "{}", vertex);

    std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts;
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY:
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
                    handleFunctionEntryVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        case Vertex::Type::REGULAR: {
            succeeding_contexts = handleRegularVertex(vertex, std::move(context));
            break;
        }
        case Vertex::Type::EXIT:
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramExitVertex(frame, *context, state);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockExitVertex(frame, *context, state);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    handleFunctionExitVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        default:
            throw std::runtime_error("Unexpected vertex type encountered.");
    }

    return succeeding_contexts;
}

void Executor::initialize(const Cfg &cfg) {
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _flattened_name_to_version.emplace(std::move(flattened_name), 0);
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

void Executor::handleFunctionEntryVertex() {
    throw std::logic_error("Not implemented yet.");
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context) {
    _context = std::move(context);
    _forked_contexts = boost::none;

    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);

    std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts;
    if (_forked_contexts.has_value()) {
        std::unique_ptr<Context> positive_context = std::move(_forked_contexts->first);
        std::unique_ptr<Context> negative_context = std::move(_forked_contexts->second);
        if (positive_context && negative_context) {
            succeeding_contexts.first = std::move(positive_context);
            succeeding_contexts.second = std::move(negative_context);
        } else if (positive_context && !negative_context) {
            succeeding_contexts.first = std::move(positive_context);
        } else if (!positive_context && negative_context) {
            succeeding_contexts.first = std::move(negative_context);
        } else {
            throw std::runtime_error("Over-approximation neither forks into positive nor negative branch, impossible.");
        }
    } else {
        succeeding_contexts.first = std::move(_context);
    }
    return succeeding_contexts;
}

void Executor::handleProgramExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int cycle = context.getCycle();
    unsigned int next_cycle = cycle + 1;
    const Cfg &cfg = frame.getCfg();

    // prepare "initial" valuations for the next cycle, relate "local" variables with each other and keep
    // "whole-program" input variables unconstrained
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _flattened_name_to_version.at(flattened_name) = 0;
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(next_cycle);
        // In an over-approximating symbolic execution, no distinction between "whole-program" input variables and
        // local/output variables is made, everything is "truly" symbolic, i.e., treated as a "whole-program" input.
        z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, it->getDataType());
        state.setSymbolicValuation(contextualized_name, symbolic_valuation);
    }

    // XXX resetting the versions locally must be done after coupling the valuations of the prior cycle into the next
    // XXX cycle, else it is not possible to retrieve the highest version of a valuation reaching the end of the cycle
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        // reset version locally
        state.setVersion(flattened_name, 0);
    }

    // XXX clear the path constraint (only possible AFTER merge, because the path constraint is embedded in the
    // XXX "ite"-expressions of the respective symbolic variables)
    state._path_constraint.clear();

    // update control-flow
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
    context.setCycle(next_cycle);
}

void Executor::handleFunctionBlockExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int next_label = frame.getReturnLabel();
    context.popFrame();
    const Frame &caller_frame = context.getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::handleFunctionExitVertex() {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // encode rhs of the assignment
    z3::expr encoded_expression = _encoder->encode(expression, *_context);

    // encode lhs of the assignment
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _flattened_name_to_version.at(flattened_name) + 1;
    // update version globally
    _flattened_name_to_version.at(flattened_name) = version;
    // update version locally
    state.setVersion(flattened_name, version);

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    // update concrete and symbolic valuations
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
    std::string type_representative_name = data_type.getName();
    const Cfg &callee = caller.getCfg(type_representative_name);
    std::string scope = frame.getScope() + "." + variable->getName();
    std::shared_ptr<Frame> callee_frame = std::make_shared<Frame>(callee, scope, return_label);
    _context->pushFrame(callee_frame);

    // update control-flow
    const Vertex &next_vertex = callee.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

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

    boost::optional<std::unique_ptr<Context>> positive_context =
            tryFork(state, path_constraint, encoded_expression, next_positive_vertex);
    boost::optional<std::unique_ptr<Context>> negative_context =
            tryFork(state, path_constraint, !encoded_expression, next_negative_vertex);
    if (positive_context && negative_context) {
        _forked_contexts = std::make_pair(std::move(*positive_context), std::move(*negative_context));
    } else if (positive_context && !negative_context) {
        _forked_contexts = std::make_pair(std::move(*positive_context), nullptr);
    } else if (!positive_context && negative_context) {
        _forked_contexts = std::make_pair(nullptr, std::move(*negative_context));
    } else {
        throw std::runtime_error("Unreachable.");
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
    // update version globally
    _flattened_name_to_version.at(flattened_name) = version;
    // update version locally
    state.setVersion(flattened_name, version);

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
    state.setSymbolicValuation(contextualized_name, expression);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

boost::optional<std::unique_ptr<Context>> Executor::tryFork(const State &state,
                                                            const std::vector<z3::expr> &path_constraint,
                                                            const z3::expr &expression, const Vertex &vertex) {
    auto logger = spdlog::get("Oa");
    SPDLOG_LOGGER_TRACE(logger, "Trying to fork on expression: {} under path constraint: {}", expression,
                        path_constraint);
    // XXX In an over-approximating symbolic execution, expression does not need to contain a "whole-program" input
    // XXX or an "unconstrained" uninterpreted constant -> always fork if path constraint and expression under current
    // XXX valuation is satisfiable!
    z3::expr_vector expressions(_solver->getContext());
    for (const z3::expr &constraint : path_constraint) {
        expressions.push_back(constraint);
    }
    expressions.push_back(expression);
    // add hard constraints, i.e., evaluate path constraint and expression under current valuations
    for (const auto &symbolic_valuation : state.getSymbolicValuations()) {
        if (symbolic_valuation.second.is_bool()) {
            expressions.push_back(_solver->makeBooleanConstant(symbolic_valuation.first) == symbolic_valuation.second);
        } else if (symbolic_valuation.second.is_int()) {
            expressions.push_back(_solver->makeIntegerConstant(symbolic_valuation.first) == symbolic_valuation.second);
        } else {
            throw std::runtime_error("Unexpected z3::sort encountered.");
        }
    }
    std::pair<z3::check_result, boost::optional<z3::model>> result = _solver->check(expressions);
    boost::optional<std::unique_ptr<Context>> forked_context = boost::none;
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
            forked_context = _context->fork(*_solver, vertex, model, expression);
            break;
        }
        case z3::unknown: {
            // XXX fall-through
        }
        default:
            throw std::runtime_error("Unexpected z3::check_result encountered.");
    }
    return forked_context;
}