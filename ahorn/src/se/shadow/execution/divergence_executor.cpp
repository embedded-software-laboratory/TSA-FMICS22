#include "se/shadow/execution/divergence_executor.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::shadow;

DivergenceExecutor::DivergenceExecutor(Solver &solver)
    : _solver(&solver), _encoder(std::make_unique<DivergenceEncoder>(*_solver)),
      _evaluator(std::make_unique<DivergenceEvaluator>(*_solver)), _flattened_name_to_version(),
      _whole_program_inputs(), _execution_status(ExecutionStatus::EXPECTED_BEHAVIOR), _context(nullptr),
      _divergent_contexts(boost::none) {}

unsigned int DivergenceExecutor::getVersion(const std::string &flattened_name) const {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    return _flattened_name_to_version.at(flattened_name);
}

void DivergenceExecutor::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

bool DivergenceExecutor::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

std::pair<DivergenceExecutor::ExecutionStatus, std::vector<std::unique_ptr<Context>>>
DivergenceExecutor::execute(Context &context) {
    auto logger = spdlog::get("Shadow");
    SPDLOG_LOGGER_TRACE(logger, "Executing context: \n{}", context);

    State &state = context.getState();
    const Vertex &vertex = state.getVertex();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    ExecutionStatus execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;
    std::vector<std::unique_ptr<Context>> divergent_contexts = std::vector<std::unique_ptr<Context>>();
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY:
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramEntryVertex(cfg, vertex, state);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockEntryVertex(cfg, vertex, state);
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
            execution_status = handleRegularVertex(vertex, context);
            if (execution_status == ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR) {
                if (_divergent_contexts.has_value()) {
                    for (std::unique_ptr<Context> &divergent_context : *_divergent_contexts) {
                        divergent_contexts.push_back(std::move(divergent_context));
                    }
                } else {
                    SPDLOG_LOGGER_TRACE(logger, "Neither additional divergent behavior in old nor new program is "
                                                "derivable.");
                    execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;
                }
            } else {
                assert(execution_status == ExecutionStatus::EXPECTED_BEHAVIOR ||
                       execution_status == ExecutionStatus::DIVERGENT_BEHAVIOR);
                assert(!_divergent_contexts.has_value());
            }
            break;
        }
        case Vertex::Type::EXIT:
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramExitVertex(frame, context, state);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockExitVertex(frame, context, state);
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
    return std::make_pair(execution_status, std::move(divergent_contexts));
}

void DivergenceExecutor::initialize(const Cfg &cfg) {
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

void DivergenceExecutor::reset() {
    _flattened_name_to_version.clear();
    _whole_program_inputs.clear();
    _execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;
    _context = nullptr;
    _divergent_contexts = boost::none;
    _encoder->reset();
    _evaluator->reset();
}

void DivergenceExecutor::handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void DivergenceExecutor::handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void DivergenceExecutor::handleFunctionEntryVertex() {
    throw std::logic_error("Not implemented yet.");
}

DivergenceExecutor::ExecutionStatus DivergenceExecutor::handleRegularVertex(const Vertex &vertex, Context &context) {
    _execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;
    _context = &context;
    _divergent_contexts = boost::none;
    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);
    return _execution_status;
}

void DivergenceExecutor::handleProgramExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int cycle = context.getCycle();
    unsigned int next_cycle = cycle + 1;
    const Cfg &cfg = frame.getCfg();

    // prepare "initial" valuations for the next cycle, relate "local" variables with each other and keep
    // "whole-program" input variables unconstrained
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        // reset version globally
        _flattened_name_to_version.at(flattened_name) = 0;
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(next_cycle);

        // distinguish between "whole-program" input variables and local/output variables
        if (isWholeProgramInput(flattened_name)) {
            z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, it->getDataType());
            state.setConcreteValuation(contextualized_name, _solver->makeRandomValue(it->getDataType()));
            state.setSymbolicValuation(contextualized_name, symbolic_valuation);
        } else {
            unsigned int highest_version = state.getHighestVersion(flattened_name);
            z3::expr concrete_valuation = state.getConcreteValuation(
                    flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle));
            z3::expr symbolic_valuation = state.getSymbolicValuation(
                    flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle));
            state.setConcreteValuation(contextualized_name, concrete_valuation);
            state.setSymbolicValuation(contextualized_name, symbolic_valuation);
        }
    }

    // XXX resetting the versions locally must be done after coupling the valuations of the prior cycle into the next
    // XXX cycle, else it is not possible to retrieve the highest version of a valuation reaching the end of the cycle
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        // reset version locally
        state.setVersion(flattened_name, 0);
    }

    // update control-flow
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
    context.setCycle(next_cycle);
}

void DivergenceExecutor::handleFunctionBlockExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int next_label = frame.getReturnLabel();
    context.popFrame();
    const Frame &caller_frame = context.getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void DivergenceExecutor::handleFunctionExitVertex() {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceExecutor::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // encode rhs of the assignment
    z3::expr encoded_expression =
            _encoder->encode(expression, *_context, DivergenceEncoder::ShadowProcessingMode::BOTH);

    // evaluate rhs of the assignment (before version increment)
    z3::expr evaluated_expression =
            _evaluator->evaluate(expression, *_context, DivergenceEvaluator::ShadowProcessingMode::BOTH);

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

void DivergenceExecutor::visit(const ir::CallInstruction &instruction) {
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

void DivergenceExecutor::visit(const ir::IfInstruction &instruction) {
    auto logger = spdlog::get("Shadow");
    const ir::Expression &expression = instruction.getExpression();

    // encode condition symbolically
    z3::expr encoded_expression =
            _encoder->encode(expression, *_context, DivergenceEncoder::ShadowProcessingMode::BOTH);

    State &state = _context->getState();
    auto *divergent_state = dynamic_cast<DivergentState *>(&state);
    assert(divergent_state != nullptr);
    // XXX check if encoded expression contains a nested shadow expression, if yes, evaluate expression under old
    // XXX and new context and compare the outputs
    if (containsShadowExpression(*divergent_state, encoded_expression)) {
        z3::expr old_evaluated_expression =
                _evaluator->evaluate(expression, *_context, DivergenceEvaluator::ShadowProcessingMode::OLD);
        z3::expr new_evaluated_expression =
                _evaluator->evaluate(expression, *_context, DivergenceEvaluator::ShadowProcessingMode::NEW);
        assert(old_evaluated_expression.is_bool() && new_evaluated_expression.is_bool());
        // i) if divergence is exposed, execution is stopped and context is added for execution during the second phase
        if ((old_evaluated_expression.is_true() && new_evaluated_expression.is_false()) ||
            (old_evaluated_expression.is_false() && new_evaluated_expression.is_true())) {
            // XXX concrete executions diverge
            SPDLOG_LOGGER_TRACE(logger,
                                "Divergent behavior (old, new): ({}, {}) after concrete evaluation of expression {} "
                                "encountered.",
                                old_evaluated_expression, new_evaluated_expression, encoded_expression);
            _execution_status = ExecutionStatus::DIVERGENT_BEHAVIOR;
        } else {
            assert((old_evaluated_expression.is_true() || old_evaluated_expression.is_false()) &&
                   (new_evaluated_expression.is_true() || new_evaluated_expression.is_false()));
            // ii) execution follows the same paths in both versions of the program -> check if divergences are possible
            assert(z3::eq(old_evaluated_expression, new_evaluated_expression));
            z3::expr old_encoded_expression =
                    _encoder->encode(expression, *_context, DivergenceEncoder::ShadowProcessingMode::OLD);
            z3::expr new_encoded_expression =
                    _encoder->encode(expression, *_context, DivergenceEncoder::ShadowProcessingMode::NEW);
            _execution_status = ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR;

            // determine control-flow
            const Frame &frame = _context->getFrame();
            const Cfg &cfg = frame.getCfg();
            const Vertex &vertex = state.getVertex();
            unsigned int label = vertex.getLabel();
            const Edge &true_edge = cfg.getTrueEdge(label);
            unsigned int next_positive_label = true_edge.getTargetLabel();
            const Vertex &next_positive_vertex = cfg.getVertex(next_positive_label);
            const Edge &false_edge = cfg.getFalseEdge(label);
            unsigned int next_negative_label = false_edge.getTargetLabel();
            const Vertex &next_negative_vertex = cfg.getVertex(next_negative_label);

            // check if at least one of the "diff" paths is feasible, i.e., !old && new and old && !new
            // for each feasible "diff" path, generate a test case exercising the divergent behavior (retrieve from
            // satisfying valuation, i.e., model generated by z3) and then continue doing bounded symbolic execution
            // in the new version in order systematically and comprehensively explore additional divergent behaviors
            tryDivergentFork(*divergent_state, !old_encoded_expression, new_encoded_expression, next_positive_vertex);
            tryDivergentFork(*divergent_state, old_encoded_expression, !new_encoded_expression, next_negative_vertex);

            // XXX continue the concolic execution of the unified program, as both versions follow the same path
            if (old_evaluated_expression.is_true()) {
                divergent_state->pushOldPathConstraint(old_encoded_expression);
                divergent_state->pushNewPathConstraint(new_encoded_expression);
                state.setVertex(cfg.getVertex(next_positive_label));
            } else {
                divergent_state->pushOldPathConstraint(!old_encoded_expression);
                divergent_state->pushNewPathConstraint(!new_encoded_expression);
                state.setVertex(cfg.getVertex(next_negative_label));
            }
        }
    } else {
        // XXX no shadow expression, resume "normal" execution
        // evaluate condition concretely
        z3::expr evaluated_expression =
                _evaluator->evaluate(expression, *_context, DivergenceEvaluator::ShadowProcessingMode::NONE);
        assert(evaluated_expression.is_bool() && (evaluated_expression.is_true() || evaluated_expression.is_false()));

        // determine control-flow
        const Frame &frame = _context->getFrame();
        const Cfg &cfg = frame.getCfg();
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
            state.pushPathConstraint(encoded_expression);
            state.setVertex(cfg.getVertex(next_positive_label));
        } else {
            state.pushPathConstraint(!encoded_expression);
            state.setVertex(cfg.getVertex(next_negative_label));
        }
    }
}

void DivergenceExecutor::visit(const ir::SequenceInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceExecutor::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceExecutor::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceExecutor::visit(const ir::HavocInstruction &instruction) {
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

void DivergenceExecutor::extractNecessaryHardConstraints(std::set<std::string> &necessary_hard_constraints,
                                                         const DivergentState &divergent_state,
                                                         const z3::expr &expression) const {
    std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
    if (uninterpreted_constants.empty()) {
        z3::expr simplified_expression = expression.simplify();
        assert(simplified_expression.is_true() || simplified_expression.is_false() ||
               simplified_expression.is_numeral());
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        z3::expr nested_expression = divergent_state.getSymbolicValuation(contextualized_name);
        std::vector<z3::expr> nested_uninterpreted_constants = _solver->getUninterpretedConstants(nested_expression);
        if (nested_uninterpreted_constants.empty()) {
            assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
            necessary_hard_constraints.insert(contextualized_name);
        } else if (nested_uninterpreted_constants.size() == 1) {
            std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
            if (contextualized_name == nested_contextualized_name) {
                necessary_hard_constraints.insert(contextualized_name);
            } else {
                necessary_hard_constraints.insert(contextualized_name);
                extractNecessaryHardConstraints(necessary_hard_constraints, divergent_state, nested_expression);
            }
        } else {
            necessary_hard_constraints.insert(contextualized_name);
            extractNecessaryHardConstraints(necessary_hard_constraints, divergent_state, nested_expression);
        }
    } else {
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            necessary_hard_constraints.insert(contextualized_name);
            extractNecessaryHardConstraints(necessary_hard_constraints, divergent_state, uninterpreted_constant);
        }
    }
}

void DivergenceExecutor::tryDivergentFork(const DivergentState &divergent_state, const z3::expr &old_encoded_expression,
                                          const z3::expr &new_encoded_expression, const Vertex &vertex) {
    auto logger = spdlog::get("Shadow");
    z3::expr_vector path_constraint(_solver->getContext());
    for (const z3::expr &constraint : divergent_state.getPathConstraint()) {
        path_constraint.push_back(constraint);
    }
    for (const z3::expr &old_constraint : divergent_state.getOldPathConstraint()) {
        path_constraint.push_back(old_constraint);
    }
    for (const z3::expr &new_constraint : divergent_state.getNewPathConstraint()) {
        path_constraint.push_back(new_constraint);
    }
    z3::expr simplified_path_constraint = z3::mk_and(path_constraint).simplify();
    SPDLOG_LOGGER_TRACE(logger, "Trying divergent fork on old: {} and new: {} under simplified path constraint: {}",
                        old_encoded_expression, new_encoded_expression, simplified_path_constraint);
    z3::expr_vector expressions(_solver->getContext());
    for (const z3::expr &constraint : path_constraint) {
        expressions.push_back(constraint);
    }
    expressions.push_back(old_encoded_expression);
    expressions.push_back(new_encoded_expression);
    // add hard constraints, i.e., evaluate path constraint and expression under current valuations
    const std::map<std::string, z3::expr> &symbolic_valuations = divergent_state.getSymbolicValuations();
    for (const auto &symbolic_valuation : symbolic_valuations) {
        if (containsShadowExpression(divergent_state, symbolic_valuation.second)) {
            // XXX skip addition of shadow expressions as it is safely possible because the expressions were lowered
            // XXX beforehand and do not contain any shadow expressions anymore
            continue;
        } else {
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
            SPDLOG_LOGGER_TRACE(logger, "Expression under path constraint is satisfiable, fork of execution context.");
            z3::model &model = *result.second;
            std::unique_ptr<Context> divergent_forked_context =
                    _context->divergentFork(*_solver, vertex, model, old_encoded_expression, new_encoded_expression);
            if (_divergent_contexts.has_value()) {
                _divergent_contexts->push_back(std::move(divergent_forked_context));
            } else {
                _divergent_contexts = std::vector<std::unique_ptr<Context>>();
                _divergent_contexts->push_back(std::move(divergent_forked_context));
            }
            break;
        }
        case z3::unknown: {
            // XXX fall-through
        }
        default:
            throw std::runtime_error("Unexpected z3::check_result encountered.");
    }
}

bool DivergenceExecutor::containsShadowExpression(const DivergentState &divergent_state,
                                                  const z3::expr &expression) const {
    // XXX check if expression is already a shadow expression, else check if it contains shadow expressions
    const std::map<std::string, std::pair<z3::expr, z3::expr>> &symbolic_shadow_valuations =
            divergent_state.getSymbolicShadowValuations();
    if (symbolic_shadow_valuations.find(expression.to_string()) != symbolic_shadow_valuations.end()) {
        return true;
    } else {
        std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
        if (uninterpreted_constants.empty()) {
            assert(expression.is_true() || expression.is_false() || expression.is_numeral());
            return false;
        } else if (uninterpreted_constants.size() == 1) {
            std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
            z3::expr nested_expression = divergent_state.getSymbolicValuation(contextualized_name);
            std::vector<z3::expr> nested_uninterpreted_constants =
                    _solver->getUninterpretedConstants(nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
                return false;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    return symbolic_shadow_valuations.find(nested_contextualized_name) !=
                           symbolic_shadow_valuations.end();
                } else {
                    return containsShadowExpression(divergent_state, nested_expression);
                }
            } else {
                return containsShadowExpression(divergent_state, nested_expression);
            }
        } else {
            bool contains_shadow_expression = false;
            for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
                if (containsShadowExpression(divergent_state, uninterpreted_constant)) {
                    contains_shadow_expression = true;
                    break;
                } else {
                    std::string contextualized_name = uninterpreted_constant.decl().name().str();
                    z3::expr nested_expression = divergent_state.getSymbolicValuation(contextualized_name);
                    if (containsShadowExpression(divergent_state, nested_expression)) {
                        contains_shadow_expression = true;
                        break;
                    }
                }
            }
            return contains_shadow_expression;
        }
    }
}