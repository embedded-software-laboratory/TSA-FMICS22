#include "se/ahorn/execution/executor.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::ahorn;

Executor::Executor(Solver &solver)
    : _solver(&solver), _encoder(std::make_unique<Encoder>(*_solver)),
      _evaluator(std::make_unique<Evaluator>(*_solver)), _flattened_name_to_version(), _whole_program_inputs(),
      _context(nullptr), _forked_context(boost::none) {}

unsigned int Executor::getVersion(const std::string &flattened_name) const {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    return _flattened_name_to_version.at(flattened_name);
}

void Executor::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

bool Executor::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
Executor::execute(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("Ahorn");
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

void Executor::handleProgramExitVertex(const Frame &frame, Context &context, State &state) {
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
            unsigned int highest_version = state.getHighestVersion(flattened_name, cycle);
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

    /*
    if (type_representative_name.find("TON") != std::string::npos) {
        // XXX handle timer explicitly
        handleTimerTON();
        _context->popFrame();
        // update control-flow by skipping body of TON and continuing in caller
        const Vertex &next_vertex = caller.getVertex(return_label);
        state.setVertex(next_vertex);
    } else {*/
    // update control-flow
    const Vertex &next_vertex = callee.getVertex(next_label);
    state.setVertex(next_vertex);
    //}
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

void Executor::tryFork(const State &state, const std::vector<z3::expr> &path_constraint, const z3::expr &expression,
                       const Vertex &vertex) {
    auto logger = spdlog::get("Ahorn");
    SPDLOG_LOGGER_TRACE(logger, "Trying to fork on expression: {} under path constraint: {}", expression,
                        path_constraint);
    // XXX check whether expression contains either a "whole-program" input or an "unconstrained" uninterpreted
    // XXX constant, e.g., resulting from a havoc instruction
    if (containsUnconstrainedUninterpretedConstant(state, expression)) {
        std::set<std::string> necessary_hard_constraints;
        z3::expr_vector expressions(_solver->getContext());
        for (const z3::expr &constraint : path_constraint) {
            // extractNecessaryHardConstraints(necessary_hard_constraints, state, constraint);
            expressions.push_back(constraint);
        }
        // extractNecessaryHardConstraints(necessary_hard_constraints, state, expression);
        expressions.push_back(expression);
        // add hard constraints, i.e., evaluate path constraint and expression under current valuations
        /*
        const std::map<std::string, z3::expr> &symbolic_valuations = state.getSymbolicValuations();
        for (const std::string &contextualized_name : necessary_hard_constraints) {
            const z3::expr &necessary_expression = symbolic_valuations.at(contextualized_name);
            if (necessary_expression.is_bool()) {
                expressions.push_back(_solver->makeBooleanConstant(contextualized_name) == necessary_expression);
            } else if (necessary_expression.is_int()) {
                expressions.push_back(_solver->makeIntegerConstant(contextualized_name) == necessary_expression);
            } else {
                throw std::runtime_error("Unexpected z3::sort encountered.");
            }
        }
        */
        // TODO: Extraction of only necessary hard constraints kills performance!
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
    z3::expr simplified_expression = expression.simplify();
    std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(simplified_expression);
    if (uninterpreted_constants.empty()) {
        assert(simplified_expression.is_true() || simplified_expression.is_false() ||
               simplified_expression.is_numeral());
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
            // XXX check for self-reference, as checking for whole-program input is not enough, because it could have
            // XXX been assigned or we have a havoc'ed variable
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

void Executor::extractNecessaryHardConstraints(std::set<std::string> &necessary_hard_constraints, const State &state,
                                               const z3::expr &expression) const {
    std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
    if (uninterpreted_constants.empty()) {
        assert(expression.is_true() || expression.is_false() || expression.is_numeral());
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        z3::expr nested_expression = state.getSymbolicValuation(contextualized_name);
        std::vector<z3::expr> nested_uninterpreted_constants = _solver->getUninterpretedConstants(nested_expression);
        if (nested_uninterpreted_constants.empty()) {
            assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
            necessary_hard_constraints.insert(contextualized_name);
        } else if (nested_uninterpreted_constants.size() == 1) {
            std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
            if (contextualized_name == nested_contextualized_name) {
                // XXX "truly" symbolic
                necessary_hard_constraints.insert(contextualized_name);
            } else {
                necessary_hard_constraints.insert(contextualized_name);
                extractNecessaryHardConstraints(necessary_hard_constraints, state, nested_expression);
            }
        } else {
            necessary_hard_constraints.insert(contextualized_name);
            extractNecessaryHardConstraints(necessary_hard_constraints, state, nested_expression);
        }
    } else {
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            necessary_hard_constraints.insert(contextualized_name);
            extractNecessaryHardConstraints(necessary_hard_constraints, state, uninterpreted_constant);
        }
    }
}

void Executor::handleTimerTON() {
    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();

    // XXX evaluate TON concretely
    std::string flattened_name_running = frame.getScope() + "." + "running";
    std::string contextualized_name_running =
            flattened_name_running + "_" +
            std::to_string(state.getHighestVersion(flattened_name_running, cycle, true)) + "__" + std::to_string(cycle);
    z3::expr concrete_valuation_running = state.getConcreteValuation(contextualized_name_running);
    assert(concrete_valuation_running.is_bool());

    std::string flattened_name_QPre = frame.getScope() + "." + "QPre";
    std::string contextualized_name_QPre = flattened_name_QPre + "_" +
                                           std::to_string(state.getHighestVersion(flattened_name_QPre, cycle, true)) +
                                           "__" + std::to_string(cycle);
    z3::expr concrete_valuation_QPre = state.getConcreteValuation(contextualized_name_QPre);
    assert(concrete_valuation_QPre.is_bool());

    std::string flattened_name_Q = frame.getScope() + "." + "Q";
    std::string contextualized_name_Q = flattened_name_Q + "_" +
                                        std::to_string(state.getHighestVersion(flattened_name_Q, cycle, true)) + "__" +
                                        std::to_string(cycle);
    z3::expr concrete_valuation_Q = state.getConcreteValuation(contextualized_name_Q);
    assert(concrete_valuation_Q.is_bool());

    std::string flattened_name_IN = frame.getScope() + "." + "IN";
    std::string contextualized_name_IN = flattened_name_IN + "_" +
                                         std::to_string(state.getHighestVersion(flattened_name_IN, cycle, true)) +
                                         "__" + std::to_string(cycle);
    z3::expr concrete_valuation_IN = state.getConcreteValuation(contextualized_name_IN);
    assert(concrete_valuation_IN.is_bool());

    if (concrete_valuation_running.is_false() && concrete_valuation_Q.is_false()) {
        // XXX State: Not Running -> valuation of "running" depends on the valuation of "IN"
        {// update running
            unsigned int version = _flattened_name_to_version.at(flattened_name_running) + 1;
            _flattened_name_to_version.at(flattened_name_running) = version;
            std::string contextualized_name =
                    flattened_name_running + "_" + std::to_string(version) + "__" + std::to_string(cycle);
            state.setConcreteValuation(contextualized_name, concrete_valuation_IN);
        }
        {// keep Q
            unsigned int version = _flattened_name_to_version.at(flattened_name_Q) + 1;
            _flattened_name_to_version.at(flattened_name_Q) = version;
            std::string contextualized_name =
                    flattened_name_Q + "_" + std::to_string(version) + "__" + std::to_string(cycle);
            state.setConcreteValuation(contextualized_name, concrete_valuation_Q);
        }
    } else if (concrete_valuation_running.is_true() && concrete_valuation_Q.is_false()) {
        // XXX: State: Running
        if (concrete_valuation_IN.is_true()) {
            {// update Q
                unsigned int version = _flattened_name_to_version.at(flattened_name_Q) + 1;
                _flattened_name_to_version.at(flattened_name_Q) = version;
                std::string contextualized_name =
                        flattened_name_Q + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_QPre);
            }
            {// keep running
                unsigned int version = _flattened_name_to_version.at(flattened_name_running) + 1;
                _flattened_name_to_version.at(flattened_name_running) = version;
                std::string contextualized_name =
                        flattened_name_running + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_running);
            }
        } else {
            assert(concrete_valuation_IN.is_false());
            {// update running
                unsigned int version = _flattened_name_to_version.at(flattened_name_running) + 1;
                _flattened_name_to_version.at(flattened_name_running) = version;
                std::string contextualized_name =
                        flattened_name_running + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_IN);
            }
            {// keep Q
                unsigned int version = _flattened_name_to_version.at(flattened_name_Q) + 1;
                _flattened_name_to_version.at(flattened_name_Q) = version;
                std::string contextualized_name =
                        flattened_name_Q + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_Q);
            }
        }
    } else {
        if (concrete_valuation_IN.is_false()) {
            {// update running
                unsigned int version = _flattened_name_to_version.at(flattened_name_running) + 1;
                _flattened_name_to_version.at(flattened_name_running) = version;
                std::string contextualized_name =
                        flattened_name_running + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_IN);
            }
            {// update Q
                unsigned int version = _flattened_name_to_version.at(flattened_name_Q) + 1;
                _flattened_name_to_version.at(flattened_name_Q) = version;
                std::string contextualized_name =
                        flattened_name_Q + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                state.setConcreteValuation(contextualized_name, concrete_valuation_IN);
            }
        }
    }

    // XXX encode TON symbolically
    z3::expr symbolic_running = _solver->makeBooleanConstant(contextualized_name_running);
    z3::expr symbolic_Q = _solver->makeBooleanConstant(contextualized_name_Q);
    z3::expr symbolic_IN = _solver->makeBooleanConstant(contextualized_name_IN);
    z3::expr symbolic_QPre = _solver->makeBooleanConstant(contextualized_name_QPre);
    z3::expr concrete_true = _solver->makeBooleanValue(true);
    z3::expr concrete_false = _solver->makeBooleanValue(false);
    {// makeTimerAutomatonQ
        z3::expr state_1 = (!symbolic_running) && (!symbolic_Q);
        z3::expr state_2 = symbolic_running && !symbolic_Q;
        z3::expr state_3 = symbolic_running && symbolic_Q;
        z3::expr trans_2 = symbolic_IN && symbolic_QPre;
        z3::expr trans_3 = !symbolic_IN;

        z3::expr result =
                z3::ite(state_1, concrete_false,
                        z3::ite(state_2, z3::ite(trans_2, concrete_true, concrete_false),
                                z3::ite(state_3, z3::ite(trans_3, concrete_false, concrete_true), concrete_false)));

        unsigned int version = _flattened_name_to_version.at(flattened_name_Q);
        std::string contextualized_name =
                flattened_name_Q + "_" + std::to_string(version) + "__" + std::to_string(cycle);
        state.setSymbolicValuation(contextualized_name, result);
    }
    {// makeTimerAutomatonRunning
        z3::expr state_1 = (!symbolic_running) && (!symbolic_Q);
        z3::expr state_2 = symbolic_running && !symbolic_Q;
        z3::expr state_3 = symbolic_running && symbolic_Q;
        const z3::expr &trans_1 = symbolic_IN;
        z3::expr trans_3 = !symbolic_IN;

        z3::expr result =
                z3::ite(state_1, z3::ite(trans_1, concrete_true, concrete_false),
                        z3::ite(state_2, concrete_true,
                                z3::ite(state_3, z3::ite(trans_3, concrete_false, concrete_true), concrete_false)));

        unsigned int version = _flattened_name_to_version.at(flattened_name_running);
        std::string contextualized_name =
                flattened_name_running + "_" + std::to_string(version) + "__" + std::to_string(cycle);
        state.setSymbolicValuation(contextualized_name, result);
    }
}