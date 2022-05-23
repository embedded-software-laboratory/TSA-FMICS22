#include "se/cbmc/execution/executor.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <stdexcept>

using namespace se::cbmc;

Executor::Executor(Solver &solver, Explorer &explorer)
    : _solver(&solver), _explorer(&explorer), _encoder(std::make_unique<Encoder>(*_solver)),
      _whole_program_inputs(std::set<std::string>()), _context(nullptr), _forked_context(boost::none) {}

bool Executor::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::execute(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("CBMC");
    SPDLOG_LOGGER_TRACE(logger, "\n{}", *context);

    unsigned int cycle = context->getCycle();
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();
    State &state = context->getState();
    const Vertex &vertex = state.getVertex();

    SPDLOG_LOGGER_INFO(logger, "{}", vertex);

    std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts;
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramEntryVertex(cfg, vertex, cycle, state, frame);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockEntryVertex(cfg, vertex, cycle, state, frame);
                    succeeding_contexts.first = std::move(context);
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    handleFunctionEntryVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected program type encountered.");
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
                    handleFunctionExitVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected program type encountered.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected vertex type encountered.");
    }

    return succeeding_contexts;
}

void Executor::initialize(const Cfg &cfg) {
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _whole_program_inputs.emplace(std::move(flattened_name));
    }
}

void Executor::handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, unsigned int cycle, State &state,
                                        const Frame &frame) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    z3::expr assumption_literal = state.getAssumptionLiteral();
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);
}

void Executor::handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, unsigned int cycle, State &state,
                                              const Frame &frame) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    z3::expr assumption_literal = state.getAssumptionLiteral();
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);
}

void Executor::handleFunctionEntryVertex() {
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
    // update control-flow
    unsigned int cycle = context->getCycle();
    unsigned int next_cycle = cycle + 1;
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    State &state = context->getState();
    state.setVertex(next_vertex);
    context->setCycle(next_cycle);

    z3::expr assumption_literal = state.getAssumptionLiteral();
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(next_cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);

    // couple "local" valuations into the next cycle as hard constraints of the entry assumption literal
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();

        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(next_cycle);

        // distinguish between "whole-program" input variables and local/output variables
        if (isWholeProgramInput(flattened_name)) {
            z3::expr unconstrained_input_valuation = _solver->makeConstant(contextualized_name, it->getDataType());
            state.pushHardConstraint(next_assumption_literal_name, contextualized_name, unconstrained_input_valuation);
        } else {
            unsigned int highest_version = state.getVersion(flattened_name);
            // XXX relating "local" valuation from prior cycle with the next cycle
            z3::expr constrained_local_valuation = _solver->makeConstant(
                    flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle),
                    it->getDataType());
            state.pushHardConstraint(next_assumption_literal_name, contextualized_name, constrained_local_valuation);
        }

        // reset "global" variable versioning
        _solver->setVersion(flattened_name, 0);
    }

    // reset "local" variable versioning
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        state.setVersion(flattened_name, 0);
    }

    return context;
}

std::unique_ptr<Context> Executor::handleFunctionBlockExitVertex(std::unique_ptr<Context> context) {
    // update control-flow
    const Frame &callee_frame = context->getFrame();
    unsigned int next_label = callee_frame.getReturnLabel();
    context->popFrame();
    State &state = context->getState();
    const Frame &caller_frame = context->getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    unsigned int cycle = context->getCycle();
    z3::expr assumption_literal = state.getAssumptionLiteral();
    const Edge &interprocedural_return_edge = caller_cfg.getInterproceduralReturnEdge(next_label);
    unsigned int call_label = interprocedural_return_edge.getCallLabel();
    // retrieve assumption literal of the actual call
    std::string caller_assumption_literal_name =
            "b_" + caller_frame.getScope() + "_" + std::to_string(call_label) + "__" + std::to_string(cycle);
    z3::expr caller_assumption_literal = _solver->makeBooleanConstant(caller_assumption_literal_name);
    // relate the next assumption literal to the actual call and the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + caller_frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);

    // XXX follow HORN/HSF encoding of calls:
    // XXX (i) Next label in caller <- label at caller call, (ii) exit of callee + effect
    // XXX the first part (i) represents the intraprocedural call-to-return edge
    // XXX the second part (ii) represents the call effect
    // z.B. P_11 <- P_10, Fb_6 (so ist das mit HORN und in HSF modelliert)
    // this relates the intraprocedural call-to-return edge and adds the effect of the call
    state.pushAssumptionLiteral(next_assumption_literal_name, caller_assumption_literal && assumption_literal);

    // add effect of call, i.e., require exit of callee as hard constraint at return in caller
    // state.pushUnknownOverApproximatingSummaryLiteral(next_assumption_literal_name, assumption_literal.to_string());

    // update assumption literal of this state
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);

    return context;
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
    // increment version "globally" and "locally" for implicit SSA-form
    unsigned int version = _solver->getVersion(flattened_name) + 1;
    _solver->setVersion(flattened_name, version);
    state.setVersion(flattened_name, version);

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    // encode assumption literal
    z3::expr assumption_literal = state.getAssumptionLiteral();
    // add the effect of this instruction
    state.pushHardConstraint(assumption_literal.to_string(), contextualized_name, encoded_expression);
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);
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

    // encode assumption literal, create an assumption literal for the entry of the callee and initialize the callee
    unsigned int cycle = _context->getCycle();
    z3::expr assumption_literal = state.getAssumptionLiteral();
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + callee_frame->getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);
}

void Executor::visit(const ir::IfInstruction &instruction) {
    auto logger = spdlog::get("CBMC");

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

    unsigned int cycle = _context->getCycle();
    z3::expr assumption_literal = state.getAssumptionLiteral();
    std::string next_positive_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_positive_label) + "__" + std::to_string(cycle);
    z3::expr next_positive_assumption_literal = _solver->makeBooleanConstant(next_positive_assumption_literal_name);
    std::string next_negative_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_negative_label) + "__" + std::to_string(cycle);
    z3::expr next_negative_assumption_literal = _solver->makeBooleanConstant(next_negative_assumption_literal_name);

    z3::expr_vector expressions(_solver->getContext());
    // initial valuations
    for (const auto &initial_valuation : state.getInitialValuations()) {
        if (initial_valuation.second.is_bool()) {
            expressions.push_back(_solver->makeBooleanConstant(initial_valuation.first) == initial_valuation.second);
        } else if (initial_valuation.second.is_int()) {
            expressions.push_back(_solver->makeIntegerConstant(initial_valuation.first) == initial_valuation.second);
        } else {
            throw std::runtime_error("Unexpected z3::sort encountered.");
        }
    }

    // control-flow constraints
    for (const auto &assumption_literals : state.getAssumptionLiterals()) {
        std::string assumption_literal_name = assumption_literals.first;
        z3::expr_vector preceding_assumption_literals(_solver->getContext());
        for (const auto &preceding_assumption_literal : assumption_literals.second) {
            preceding_assumption_literals.push_back(preceding_assumption_literal);
        }
        expressions.push_back(z3::implies(_solver->makeBooleanConstant(assumption_literal_name),
                                          z3::mk_or(preceding_assumption_literals).simplify()));
    }

    // assumption constraints
    for (const auto &assumptions : state.getAssumptions()) {
        std::string assumption_literal_name = assumptions.first;
        z3::expr_vector assumption_expressions(_solver->getContext());
        for (const auto &assumption : assumptions.second) {
            assumption_expressions.push_back(assumption);
        }
        expressions.push_back(
                z3::implies(_solver->makeBooleanConstant(assumption_literal_name), z3::mk_and(assumption_expressions)));
    }

    // hard constraints
    for (const auto &hard_constraints : state.getHardConstraints()) {
        std::string assumption_literal_name = hard_constraints.first;
        z3::expr_vector hard_constraint_expressions(_solver->getContext());
        for (const auto &hard_constraint_expression : hard_constraints.second) {
            if (hard_constraint_expression.second.is_bool()) {
                hard_constraint_expressions.push_back(_solver->makeBooleanConstant(hard_constraint_expression.first) ==
                                                      hard_constraint_expression.second);
            } else if (hard_constraint_expression.second.is_int()) {
                hard_constraint_expressions.push_back(_solver->makeIntegerConstant(hard_constraint_expression.first) ==
                                                      hard_constraint_expression.second);
            } else {
                throw std::runtime_error("Unsupported z3::sort encountered.");
            }
        }
        expressions.push_back(z3::implies(_solver->makeBooleanConstant(assumption_literal_name),
                                          z3::mk_and(hard_constraint_expressions)));
    }

    // check if either "true/positive" or "false/negative" or both paths are realizable
    z3::expr_vector assumptions(_solver->getContext());
    assumptions.push_back(next_positive_assumption_literal);
    expressions.push_back(z3::implies(next_positive_assumption_literal, assumption_literal));
    expressions.push_back(z3::implies(next_positive_assumption_literal, encoded_expression));

    SPDLOG_LOGGER_TRACE(logger, "Querying solver with expressions:\n{}\nunder assumptions:\n{}", expressions,
                        assumptions);

    std::pair<z3::check_result, boost::optional<z3::model>> result =
            _solver->checkUnderAssumptions(expressions, assumptions);

    switch (result.first) {
        case z3::unsat: {
            assert(!result.second.has_value());
            SPDLOG_LOGGER_TRACE(logger,
                                "True/positive choice {} is unsatisfiable, false/negative choice {} must be "
                                "satisfiable (no need to check).",
                                next_positive_assumption_literal_name, next_negative_assumption_literal_name);
            {// XXX debug
                std::string potential_negative_goal =
                        "b_" + frame.getScope() + "_" + std::to_string(next_negative_label);
                if (_explorer->hasGoal(potential_negative_goal)) {
                    // TODO: create test case
                    SPDLOG_LOGGER_INFO(logger, "Hit negative goal {}", potential_negative_goal);
                    _explorer->removeGoal(potential_negative_goal);
                }
            }
            // update control-flow
            state.setVertex(next_negative_vertex);

            // relate the next assumption literal to the current assumption literal
            state.pushAssumptionLiteral(next_negative_assumption_literal_name, assumption_literal);
            // XXX the effect is pushed into the succeeding block
            state.pushAssumption(next_negative_assumption_literal_name, !encoded_expression);
            state.setAssumptionLiteral(next_negative_assumption_literal);
            break;
        }
        case z3::sat: {
            SPDLOG_LOGGER_TRACE(logger,
                                "True/positive choice is {} satisfiable, check if false/negative choice {} is "
                                "satisfiable and execution context should be forked.",
                                next_positive_assumption_literal_name, next_negative_assumption_literal_name);
            {// XXX debug
                std::string potential_positive_goal =
                        "b_" + frame.getScope() + "_" + std::to_string(next_positive_label);
                if (_explorer->hasGoal(potential_positive_goal)) {
                    // TODO: create test case
                    SPDLOG_LOGGER_INFO(logger, "Hit positive goal {}", potential_positive_goal);
                    _explorer->removeGoal(potential_positive_goal);
                }
                z3::model &model = *result.second;
                // SPDLOG_LOGGER_TRACE(logger, "\n{}", model);
            }

            // check if false/negative choice is satisfiable and execution context should be forked
            assumptions.pop_back();
            assumptions.push_back(next_negative_assumption_literal);
            expressions.pop_back();
            expressions.pop_back();
            expressions.push_back(z3::implies(next_negative_assumption_literal, assumption_literal));
            expressions.push_back(z3::implies(next_negative_assumption_literal, !encoded_expression));

            result = _solver->checkUnderAssumptions(expressions, assumptions);

            switch (result.first) {
                case z3::unsat: {
                    assert(!result.second.has_value());
                    SPDLOG_LOGGER_TRACE(logger,
                                        "False/negative choice {} is unsatisfiable, no fork of execution "
                                        "context.",
                                        next_negative_assumption_literal_name);
                    break;
                }
                case z3::sat: {
                    SPDLOG_LOGGER_TRACE(logger, "False/negative choice {} is satisfiable, fork of execution context.",
                                        next_negative_assumption_literal_name);
                    {// XXX debug
                        std::string potential_negative_goal =
                                "b_" + frame.getScope() + "_" + std::to_string(next_negative_label);
                        if (_explorer->hasGoal(potential_negative_goal)) {
                            // TODO: create test case
                            SPDLOG_LOGGER_INFO(logger, "Hit negative goal {}", potential_negative_goal);
                            _explorer->removeGoal(potential_negative_goal);
                        }
                        z3::model &model = *result.second;
                        // SPDLOG_LOGGER_TRACE(logger, "\n{}", model);
                    }
                    _forked_context = _context->fork(next_negative_vertex, next_negative_assumption_literal,
                                                     next_negative_assumption_literal_name, !encoded_expression);
                    break;
                }
                case z3::unknown: {
                    // XXX fall-through
                }
                default:
                    throw std::runtime_error("Unexpected z3::check_result encountered.");
            }

            // update control-flow
            state.setVertex(next_positive_vertex);

            // relate the next assumption literal to the current assumption literal
            state.pushAssumptionLiteral(next_positive_assumption_literal_name, assumption_literal);
            // XXX the effect is pushed into the succeeding block
            state.pushAssumption(next_positive_assumption_literal_name, encoded_expression);
            state.setAssumptionLiteral(next_positive_assumption_literal);

            break;
        }
        case z3::unknown: {
            // XXX fall-through
        }
        default:
            throw std::runtime_error("Unexpected z3::check_result encountered.");
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
    // increment version "globally" and "locally" for implicit SSA-form
    unsigned int version = _solver->getVersion(flattened_name) + 1;
    _solver->setVersion(flattened_name, version);
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

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    // encode assumption literal
    z3::expr assumption_literal = state.getAssumptionLiteral();
    // add the effect of this instruction
    state.pushHardConstraint(assumption_literal.to_string(), contextualized_name, expression);
    // relate the next assumption literal to the current assumption literal
    std::string next_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
    z3::expr next_assumption_literal = _solver->makeBooleanConstant(next_assumption_literal_name);
    state.setAssumptionLiteral(next_assumption_literal);
}
