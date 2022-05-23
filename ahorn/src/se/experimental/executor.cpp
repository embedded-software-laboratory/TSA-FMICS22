#include "se/experimental/executor.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"
#include "ir/instruction/while_instruction.h"
#include "se/experimental/encoder.h"
#include "se/experimental/evaluator.h"
#include "se/experimental/expression/symbolic_expression.h"
#include "utilities/ostream_operators.h"

#include "boost/optional/optional_io.hpp"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"
#include "se/simulator/execution/executor.h"

using namespace se;

Executor::Executor(Configuration &configuration, Manager &manager)
    : _configuration(&configuration), _execution_status(ExecutionStatus::EXPECTED_BEHAVIOR), _manager(&manager),
      _evaluator(std::make_unique<Evaluator>(*_configuration, *_manager)),
      _encoder(std::make_unique<Encoder>(*_configuration, *_manager)),
      _summarizer(std::make_unique<Summarizer>(*_manager)), _context(nullptr), _forked_context(boost::none),
      _divergent_contexts(boost::none) {}

std::ostream &Executor::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "Summarizer: " << *_summarizer << "\n";
    str << ")";
    return os << str.str();
}

std::pair<Executor::ExecutionStatus, std::vector<std::unique_ptr<Context>>>
Executor::execute(std::unique_ptr<Context> context) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Executing: {}", *context);

    unsigned int cycle = context->getCycle();
    const State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Frame &frame = context->getFrame();
    const Cfg &cfg = frame.getCfg();

    ExecutionStatus execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;
    std::vector<std::unique_ptr<Context>> contexts;
    switch (vertex.getType()) {
        case Vertex::Type::ENTRY: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    std::unique_ptr<Context> succeeding_context = handleProgramEntry(cfg, vertex, std::move(context));
                    contexts.push_back(std::move(succeeding_context));
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK:
                case Cfg::Type::FUNCTION: {
                    std::unique_ptr<Context> succeeding_context =
                            handleFunctionBlockEntry(cfg, vertex, std::move(context));
                    contexts.push_back(std::move(succeeding_context));
                    break;
                }
                default:
                    throw std::logic_error("Not implemented yet.");
            }
            break;
        }
        case Vertex::Type::REGULAR: {
            switch (_configuration->getExecutionMode()) {
                case Configuration::ExecutionMode::COMPOSITIONAL: {
                    std::pair<Executor::ExecutionStatus, std::vector<std::unique_ptr<Context>>> execution_result =
                            handleRegularVertex(vertex, std::move(context));
                    assert(execution_result.first == ExecutionStatus::EXPECTED_BEHAVIOR);
                    for (std::unique_ptr<Context> &succeeding_context : execution_result.second) {
                        contexts.push_back(std::move(succeeding_context));
                    }
                    break;
                }
                case Configuration::ExecutionMode::SHADOW: {
                    std::pair<Executor::ExecutionStatus, std::vector<std::unique_ptr<Context>>> execution_result =
                            handleRegularVertex(vertex, std::move(context));
                    switch (execution_result.first) {
                        case ExecutionStatus::EXPECTED_BEHAVIOR: {
                            // XXX continue execution
                            for (std::unique_ptr<Context> &succeeding_context : execution_result.second) {
                                contexts.push_back(std::move(succeeding_context));
                            }
                            break;
                        }
                        case ExecutionStatus::DIVERGENT_BEHAVIOR: {
                            execution_status = ExecutionStatus::DIVERGENT_BEHAVIOR;
                            for (std::unique_ptr<Context> &divergent_context : execution_result.second) {
                                contexts.push_back(std::move(divergent_context));
                            }
                            break;
                        }
                        case ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR: {
                            throw std::logic_error("Not implemented yet.");
                            break;
                        }
                        default:
                            throw std::runtime_error("Unexpected execution status encountered.");
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Invalid execution mode configured.");
            }
            break;
        }
        case Vertex::Type::EXIT:
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    std::chrono::time_point<std::chrono::system_clock> begin_time_point =
                            std::chrono::system_clock::now();
                    std::unique_ptr<Context> succeeding_context = handleProgramExit(cfg, vertex, std::move(context));
                    contexts.push_back(std::move(succeeding_context));
                    using namespace std::literals;
                    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
                    std::cout << "Elapsed time (handleProgramExit): " << elapsed_time << "ms" << std::endl;
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    std::unique_ptr<Context> succeeding_context =
                            handleFunctionBlockExit(cfg, vertex, std::move(context));
                    contexts.push_back(std::move(succeeding_context));
                    break;
                }
                case Cfg::Type::FUNCTION:
                default:
                    throw std::logic_error("Not implemented yet.");
            }
            break;
    }

    return std::make_pair(execution_status, std::move(contexts));
}

std::unique_ptr<Context> Executor::handleProgramEntry(const Cfg &cfg, const Vertex &vertex,
                                                      std::unique_ptr<Context> context) {
    unsigned int cycle = context->getCycle();
    unsigned int label = vertex.getLabel();
    State &state = context->getState();
    const Frame &frame = context->getFrame();

    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
            // relate the next assumption literal to the current assumption literal
            std::string next_assumption_literal_name =
                    "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
            state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
            z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
            std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                    std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
            state.setAssumptionLiteral(next_assumption_literal);
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    return context;
}

std::unique_ptr<Context> Executor::handleFunctionBlockEntry(const Cfg &cfg, const Vertex &vertex,
                                                            std::unique_ptr<Context> context) {
    // TODO (21.12.21): Check here if there are any applicable summaries?
    unsigned int cycle = context->getCycle();
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(vertex.getLabel());
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    State &state = context->getState();
    state.setVertex(next_vertex);

    const Frame &frame = context->getFrame();

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
            // relate the next assumption literal to the current assumption literal
            std::string next_assumption_literal_name =
                    "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
            state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
            z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
            std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                    std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
            state.setAssumptionLiteral(next_assumption_literal);
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    return context;
}

std::pair<Executor::ExecutionStatus, std::vector<std::unique_ptr<Context>>>
Executor::handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context) {
    _context = std::move(context);
    _forked_context = boost::none;
    _divergent_contexts = boost::none;
    _execution_status = ExecutionStatus::EXPECTED_BEHAVIOR;

    std::vector<std::unique_ptr<Context>> succeeding_contexts;
    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);

    if (_divergent_contexts.has_value()) {
        std::vector<std::unique_ptr<Context>> divergent_contexts = std::move(*_divergent_contexts);
        // either the input exposes a divergence, i.e., two different paths are feasible, or both program versions
        // take the same path but divergences are possible
        switch (_execution_status) {
            case ExecutionStatus::EXPECTED_BEHAVIOR: {
                throw std::runtime_error("Unexpected execution status encountered, expected (potential) divergent "
                                         "behavior.");
            }
            case ExecutionStatus::DIVERGENT_BEHAVIOR: {
                return std::make_pair(_execution_status, std::move(divergent_contexts));
                break;
            }
            case ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR: {
                throw std::logic_error("Not implemented yet.");
            }
            default:
                throw std::runtime_error("Invalid execution status.");
        }
    } else {
        succeeding_contexts.push_back(std::move(_context));
        if (_forked_context.has_value()) {
            succeeding_contexts.push_back(std::move(*_forked_context));
        }
        return std::make_pair(_execution_status, std::move(succeeding_contexts));
    }
}

// TODO (10.01.2022): It is important to note that the merge at exit occurs before processing the context in
//  handleProgramExit.
std::unique_ptr<Context> Executor::handleProgramExit(const Cfg &cfg, const Vertex &vertex,
                                                     std::unique_ptr<Context> context) {
    auto logger = spdlog::get("Symbolic Execution");

    // TODO (10.01.2022): Expressions are not correctly lowered or removed
    unsigned int cycle = context->getCycle();
    unsigned int next_cycle = cycle + 1;

    State &state = context->getState();
    unsigned int label = vertex.getLabel();
    const ir::Interface &interface = cfg.getInterface();
    // XXX introduce fresh symbolic constants for "whole-program" inputs, lower expressions and propagate constants
    std::map<std::string, std::shared_ptr<Expression>> contextualized_name_to_symbolic_expression;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        unsigned int highest_version = state.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
        std::string contextualized_name = flattened_name + "_0__" + std::to_string(next_cycle);
        std::shared_ptr<Expression> expression = state.getSymbolicExpression(
                flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle));

        std::shared_ptr<Expression> lowered_expression = context->lowerExpression(
                *_manager, expression, _id_to_uninterpreted_constants, _id_to_lowered_expression);

        std::shared_ptr<Expression> concrete_expression = state.getConcreteExpression(
                flattened_name + "_" + std::to_string(highest_version) + "__" + std::to_string(cycle));
        state.setConcreteExpression(contextualized_name, concrete_expression);

        // distinguish between "whole-program" input variables and local/output variables
        if (_manager->isWholeProgramInput(flattened_name)) {
            state.setSymbolicExpression(contextualized_name,
                                        std::make_shared<SymbolicExpression>(
                                                _manager->makeConstant(contextualized_name, it->getDataType())));
        } else {
            state.setSymbolicExpression(contextualized_name, lowered_expression);
        }
    }

    // TODO (15.12.21): Was ist mit den expressions im path constraint? diese müssen auch ersetzt werden
    /*const std::vector<std::shared_ptr<Expression>> &path_constraint = state.getPathConstraint();
    for (const std::shared_ptr<Expression> &constraint : path_constraint) {
        std::shared_ptr<Expression> lowered_constraint = context->lowerExpression(*_manager, constraint);
    }*/

    // TODO (26.01.2022): What about non-deterministic values?
    // XXX remove intermediate versions
    state.removeIntermediateVersions(*_manager, cycle);

    // XXX clear the path constraint (only possible AFTER merge, because the path constraint is embedded in the
    // XXX "ite"-expressions of the respective symbolic variables)
    state.clearPathConstraint();

    Frame &frame = context->getFrame();
    frame.popLocalPathConstraints();
    unsigned int next_label = frame.getLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
    context->setCycle(next_cycle);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
            // relate the next assumption literal to the current assumption literal
            std::string next_assumption_literal_name =
                    "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(next_cycle);
            state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
            z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
            std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                    std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
            state.setAssumptionLiteral(next_assumption_literal);
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    SPDLOG_LOGGER_TRACE(logger, "mmm {}", *context);

    return context;
}

std::unique_ptr<Context> Executor::handleFunctionBlockExit(const Cfg &cfg, const Vertex &vertex,
                                                           std::unique_ptr<Context> context) {
    // TODO (21.12.21): Summarize?
    unsigned int label = vertex.getLabel();
    const Frame &callee_frame = context->getFrame();
    unsigned int next_label = callee_frame.getLabel();

    // TODO (03.01.2022): Check if a summary already exists?
    switch (_configuration->getSummarizationMode()) {
        case Configuration::SummarizationMode::NONE:
            break;
        case Configuration::SummarizationMode::FUNCTION_BLOCK: {
            _summarizer->summarize(*context);
            break;
        }
        default:
            throw std::runtime_error("Unexpected summarization mode encountered.");
    }

    context->popFrame();

    State &state = context->getState();
    const Frame &caller_frame = context->getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            // TODO (12.01.2022): i) create an assumption literal for the function block exit and ii) create an assumption
            //  literal for the next_label to which this context resumes in the caller's context and imply this assumption
            //  literal
            // TODO (14.01.2022): Wird es nicht immer der Fall sein, dass es bereits eine assumption literal für i) gibt?
            //  (auf grund des mergens?) -> ja, aber nicht immer mergen wir wenn wir einen merge point erreichen (weil z.b.
            //  nur ein context den merge point erreicht)
            // TODO (17.01.2022): Unabhängig ob es der exit ist oder nicht, es wird immer eine assumption literal bereits
            //  existieren wenn wir mergen, oder nicht? -> ja, wenn wir mergen, aber nicht immer wenn exit erreicht wird.
            // TODO (17.01.2022): Es wird auch immer eine assumption literal existieren wenn es der target von einem return
            //  ist, i.e., wir wieder zum caller context zurückkehren.

            // TODO (17.01.2022): Modellierung des Call-Effects mit Assumption Literalen: Next label in caller <- label at
            //  caller call, exit of callee + effect, z.B. P_11 <- P_10, Fb_6 (so ist das mit HORN und in HSF modelliert)

            // TODO (17.01.2022): An assumption literal exists already because the label is either a return from a call or a
            //  merge point -> this is not true, while it is a merge point only one context might reach the exit in a cycle,
            //  hence no merge would occur and therefore also no assumption literal would be generated.
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
            unsigned int cycle = context->getCycle();
            const Edge &interprocedural_return_edge = caller_cfg.getInterproceduralReturnEdge(next_label);
            unsigned int call_label = interprocedural_return_edge.getCallLabel();
            // retrieve assumption literal of the actual call
            std::string caller_assumption_literal_name =
                    "b_" + caller_frame.getScope() + "_" + std::to_string(call_label) + "__" + std::to_string(cycle);
            z3::expr z3_caller_assumption_literal = _manager->makeBooleanConstant(caller_assumption_literal_name);
            std::shared_ptr<AssumptionLiteralExpression> caller_assumption_literal =
                    std::make_shared<AssumptionLiteralExpression>(z3_caller_assumption_literal);
            // relate the next assumption literal to the actual call and the current assumption literal
            std::string next_assumption_literal_name =
                    "b_" + caller_frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
            // this relates the intraprocedural call-to-return edge
            state.pushAssumptionLiteral(next_assumption_literal_name, caller_assumption_literal);

            // add effect of call, i.e., require exit of callee as hard constraint at return in caller
            state.pushUnknownOverApproximatingSummaryLiteral(next_assumption_literal_name,
                                                             assumption_literal->getZ3Expression().to_string());

            // update assumption literal of this state
            z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
            std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                    std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
            state.setAssumptionLiteral(next_assumption_literal);
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    return context;
}

void Executor::tryFork(std::shared_ptr<Expression> expression, const Vertex &next_vertex) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Trying to fork on: {}", *expression);

    const State &state = _context->getState();
    const std::vector<std::shared_ptr<Expression>> &path_constraint = state.getPathConstraint();

    // TODO (21.01.2022): It should also be possible to fork on non-deterministic constants resulting from havoc
    //  instructions.
    if (_context->containsWholeProgramInput(*_manager, *expression)) {
        std::set<std::string> necessary_hard_constraints;
        // XXX to find a suitable interpretation for the "whole-program" inputs, the constraints need to be lowered
        z3::expr_vector constraints(_manager->getZ3Context());
        for (const std::shared_ptr<Expression> &constraint : path_constraint) {
            // std::shared_ptr<Expression> lowered_expression = _context->lowerExpression(*_manager, constraint);
            std::shared_ptr<Expression> lowered_expression = constraint;
            std::set<std::string> necessary_hard_constraints_for_constraint =
                    _context->extractNecessaryHardConstraints(*_manager, lowered_expression->getZ3Expression());
            for (const std::string &contextualized_name : necessary_hard_constraints_for_constraint) {
                necessary_hard_constraints.emplace(contextualized_name);
            }
            constraints.push_back(lowered_expression->getZ3Expression().simplify());
        }

        SPDLOG_LOGGER_TRACE(logger, "Lowering expression: {}", *expression);
        std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();
        std::shared_ptr<Expression> lowered_expression = _context->lowerExpression(
                *_manager, expression, _id_to_uninterpreted_constants, _id_to_lowered_expression);
        using namespace std::literals;
        auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
        std::cout << "Elapsed time (Lowering expression): " << elapsed_time << "ms" << std::endl;

        SPDLOG_LOGGER_TRACE(logger, "to lowered expression: {}.", *lowered_expression);
        for (const std::string &contextualized_name :
             _context->extractNecessaryHardConstraints(*_manager, lowered_expression->getZ3Expression())) {
            necessary_hard_constraints.emplace(contextualized_name);
        }
        constraints.push_back(lowered_expression->getZ3Expression().simplify());
        SPDLOG_LOGGER_TRACE(logger, "{}", constraints);
        SPDLOG_LOGGER_TRACE(logger, "Necessary hard constraints: {}", necessary_hard_constraints);
        for (const std::string &contextualized_name : necessary_hard_constraints) {
            std::shared_ptr<Expression> symbolic_expression = state.getSymbolicExpression(contextualized_name);
            z3::expr z3_expression = symbolic_expression->getZ3Expression();
            if (z3_expression.is_bool()) {
                constraints.push_back(_manager->makeBooleanConstant(contextualized_name) == z3_expression);
            } else {
                assert(z3_expression.is_int());
                constraints.push_back(_manager->makeIntegerConstant(contextualized_name) == z3_expression);
            }
        }

        // TODO (10.01.2022): Currently check does not consider the context, add the dependent valuations.
        std::pair<z3::check_result, boost::optional<z3::model>> result = _manager->check(constraints);

        switch (result.first) {
            case z3::unsat: {
                assert(!result.second.has_value());
                SPDLOG_LOGGER_TRACE(logger, "{} and path constraint are unsat, no fork of execution context.",
                                    *expression);
                break;
            }
            case z3::sat: {
                // XXX fork
                z3::model &z3_model = *result.second;
                SPDLOG_LOGGER_TRACE(logger, "Path constraint and expression is sat, resulting model: {}", z3_model);
                _forked_context = _context->fork(*_manager, z3_model);

                State &forked_state = (*_forked_context)->getState();

                // XXX take shared ownership of forked expression
                forked_state.pushPathConstraint(lowered_expression);
                (*_forked_context)->pushLocalPathConstraint(std::move(expression));

                // XXX update next vertex
                forked_state.setVertex(std::move(next_vertex));
                break;
            }
            case z3::unknown:
            default:
                throw std::logic_error("Invalid z3::check_result.");
        }
    } else {
        SPDLOG_LOGGER_TRACE(logger, "Expression does not contain a whole-program input, no fork of execution context.");
    }
}

void Executor::tryDivergentFork(std::shared_ptr<Expression> old_expression,
                                std::shared_ptr<Expression> new_expression) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Trying divergent fork on old: {} and new: {}", *old_expression, *new_expression);
    throw std::logic_error("Not implemented yet.");
}

bool Executor::containsShadowExpression(const Expression &expression) const {
    const State &state = _context->getState();
    const z3::expr &z3_expression = expression.getZ3Expression();
    std::vector<z3::expr> uninterpreted_constants = _manager->getUninterpretedConstantsFromZ3Expression(z3_expression);
    if (uninterpreted_constants.empty()) {
        return false;
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        std::shared_ptr<Expression> nested_expression = state.getSymbolicExpression(contextualized_name);
        const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
        std::vector<z3::expr> nested_uninterpreted_constants =
                _manager->getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
        if (nested_uninterpreted_constants.empty()) {
            assert(expression.getKind() != Expression::Kind::SHADOW_EXPRESSION);
            return false;
        } else if (nested_uninterpreted_constants.size() == 1) {
            std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
            if (contextualized_name == nested_contextualized_name) {
                return false;
            } else {
                return containsShadowExpression(*nested_expression);
            }
        } else {
            return containsShadowExpression(*nested_expression);
        }
    } else {
        bool contains_shadow_expression = false;
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.to_string();
            std::size_t shadow_position = contextualized_name.find("shadow");
            if (shadow_position == std::string::npos) {
                contains_shadow_expression =
                        containsShadowExpression(*state.getSymbolicExpression(contextualized_name));
                if (contains_shadow_expression) {
                    break;
                }
            } else {
                contains_shadow_expression = true;
                break;
            }
        }
        return contains_shadow_expression;
    }
}

void Executor::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // encode rhs of the assignment
    std::shared_ptr<Expression> encoded_expression = _encoder->encode(expression, *_context);
    z3::expr rhs_z3_expression = encoded_expression->getZ3Expression();

    // evaluate rhs of the assignment (before version increment)
    std::shared_ptr<Expression> evaluated_expression = _evaluator->evaluate(expression, *_context);

    // encode lhs of the assignment
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = _context->getFlattenedName(name);

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _manager->getVersion(flattened_name) + 1;
    _manager->setVersion(flattened_name, version);

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    std::shared_ptr<ir::Variable> variable = nullptr;
    if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
        variable = variable_access->getVariable();
    } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
        variable = field_access->getVariableAccess().getVariable();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
    assert(variable != nullptr);
    const auto &data_type = variable->getDataType();
    z3::expr lhs_z3_expression = _manager->makeConstant(contextualized_name, data_type);

    // update symbolic expression
    state.setSymbolicExpression(contextualized_name, std::make_shared<SymbolicExpression>(rhs_z3_expression));

    // TODO (25.01.2022): How to propagate change expressions / shadow variables in the concrete store?
    // update concrete expression
    switch (_configuration->getExecutionMode()) {
        case Configuration::ExecutionMode::COMPOSITIONAL: {
            state.setConcreteExpression(contextualized_name, std::move(evaluated_expression));
            break;
        }
        case Configuration::ExecutionMode::SHADOW:
            if (evaluated_expression->getKind() == Expression::Kind::SHADOW_EXPRESSION) {
                std::shared_ptr<ShadowExpression> shadow_expression =
                        std::dynamic_pointer_cast<ShadowExpression>(evaluated_expression);
                state.setConcreteExpression(contextualized_name,
                                            std::make_shared<SymbolicExpression>(shadow_expression->getZ3Expression()));
            } else {
                state.setConcreteExpression(contextualized_name, std::move(evaluated_expression));
            }
            break;
        default:
            throw std::runtime_error("Invalid execution mode configured.");
    }

    // update control-flow
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();

    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            // encode assumption literal
            switch (_configuration->getBlockEncoding()) {
                case Configuration::BlockEncoding::SINGLE: {
                    // TODO (18.01.2022): What about assignment being a merge point? Or a successor of an if-instruction?
                    std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
                    // add the effect of this instruction
                    state.pushHardConstraint(assumption_literal->getZ3Expression().to_string(), contextualized_name,
                                             rhs_z3_expression);
                    // relate the next assumption literal to the current assumption literal
                    std::string next_assumption_literal_name =
                            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
                    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
                    z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
                    std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                            std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
                    state.setAssumptionLiteral(next_assumption_literal);
                    break;
                }
                case Configuration::BlockEncoding::BASIC:
                    break;
                default:
                    throw std::runtime_error("Invalid block encoding configured.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }
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

    // TODO (19.01.2022): Checking whether a summary is applicable or not should occur here, as the intraprocedural
    //  call-to-return edge can than be taken
    switch (_configuration->getSummarizationMode()) {
        case Configuration::SummarizationMode::NONE:
            break;
        case Configuration::SummarizationMode::FUNCTION_BLOCK: {
            _summarizer->findApplicableSummary(*_context);
            break;
        }
        default:
            throw std::runtime_error("Unexpected summarization mode encountered.");
    }
    // TODO (21.01.2022): Use the result from summary checking to skip execution?

    const Vertex &next_vertex = callee.getVertex(next_label);
    state.setVertex(next_vertex);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            // encode assumption literal, create an assumption literal for the entry of the callee and initialize the callee
            unsigned int cycle = _context->getCycle();
            switch (_configuration->getBlockEncoding()) {
                case Configuration::BlockEncoding::SINGLE: {
                    std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
                    // relate the next assumption literal to the current assumption literal
                    std::string next_assumption_literal_name = "b_" + callee_frame->getScope() + "_" +
                                                               std::to_string(next_label) + "__" +
                                                               std::to_string(cycle);
                    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
                    z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
                    std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                            std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
                    state.setAssumptionLiteral(next_assumption_literal);
                    break;
                }
                case Configuration::BlockEncoding::BASIC:
                    break;
                default:
                    throw std::runtime_error("Invalid block encoding configured.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }
}

void Executor::visit(const ir::IfInstruction &instruction) {
    auto logger = spdlog::get("Symbolic Execution");

    const ir::Expression &expression = instruction.getExpression();
    switch (_configuration->getExecutionMode()) {
        case Configuration::ExecutionMode::COMPOSITIONAL: {
            // evaluate condition
            std::shared_ptr<Expression> evaluated_expression = _evaluator->evaluate(expression, *_context);
            z3::expr z3_evaluated_expression = evaluated_expression->getZ3Expression();
            assert(z3_evaluated_expression.is_bool() &&
                   (z3_evaluated_expression.is_true() || z3_evaluated_expression.is_false()));

            // encode condition
            std::shared_ptr<Expression> encoded_expression = _encoder->encode(expression, *_context);
            std::shared_ptr<Expression> negated_encoded_expression(nullptr);
            switch (encoded_expression->getKind()) {
                case Expression::Kind::SYMBOLIC_EXPRESSION: {
                    z3::expr z3_negated_encoded_expression = !encoded_expression->getZ3Expression();
                    negated_encoded_expression = std::make_shared<SymbolicExpression>(z3_negated_encoded_expression);
                    break;
                }
                case Expression::Kind::CONCRETE_EXPRESSION:
                case Expression::Kind::SHADOW_EXPRESSION:
                default:
                    throw std::logic_error("Not implemented yet.");
            }
            assert(negated_encoded_expression != nullptr);

            const Frame &frame = _context->getFrame();
            const Cfg &cfg = frame.getCfg();
            State &state = _context->getState();
            const Vertex &vertex = state.getVertex();
            unsigned int label = vertex.getLabel();
            const Edge &true_edge = cfg.getTrueEdge(label);
            const Edge &false_edge = cfg.getFalseEdge(label);
            unsigned int next_positive_label = true_edge.getTargetLabel();
            unsigned int next_negative_label = false_edge.getTargetLabel();

            // TODO (05.01.2022): Add assumption expression in tryFork and in these branch cases
            if (z3_evaluated_expression.is_true()) {
                tryFork(negated_encoded_expression, cfg.getVertex(next_negative_label));
                std::shared_ptr<Expression> lowered_encoded_expression = _context->lowerExpression(
                        *_manager, encoded_expression, _id_to_uninterpreted_constants, _id_to_lowered_expression);
                state.pushPathConstraint(lowered_encoded_expression);
                _context->pushLocalPathConstraint(encoded_expression);
                state.setVertex(cfg.getVertex(next_positive_label));
            } else {
                tryFork(encoded_expression, cfg.getVertex(next_positive_label));
                std::shared_ptr<Expression> lowered_negated_encoded_expression =
                        _context->lowerExpression(*_manager, negated_encoded_expression, _id_to_uninterpreted_constants,
                                                  _id_to_lowered_expression);
                state.pushPathConstraint(lowered_negated_encoded_expression);
                _context->pushLocalPathConstraint(negated_encoded_expression);
                state.setVertex(cfg.getVertex(next_negative_label));
            }

            // TODO (18.01.2022): Should I introduce intermediate basic blocks for the target of the if holding only the
            //  assume? I think I should, this was one lesson learned when using crab for generating invariants!
            switch (_configuration->getEncodingMode()) {
                case Configuration::EncodingMode::NONE:
                    break;
                case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
                    unsigned int cycle = _context->getCycle();
                    switch (_configuration->getBlockEncoding()) {
                        case Configuration::BlockEncoding::SINGLE: {
                            std::shared_ptr<AssumptionLiteralExpression> assumption_literal =
                                    state.getAssumptionLiteral();
                            std::string next_positive_assumption_literal_name = "b_" + frame.getScope() + "_" +
                                                                                std::to_string(next_positive_label) +
                                                                                "__" + std::to_string(cycle);
                            std::shared_ptr<AssumptionLiteralExpression> next_positive_assumption_literal =
                                    std::make_shared<AssumptionLiteralExpression>(
                                            _manager->makeBooleanConstant(next_positive_assumption_literal_name));
                            std::string next_negative_assumption_literal_name = "b_" + frame.getScope() + "_" +
                                                                                std::to_string(next_negative_label) +
                                                                                "__" + std::to_string(cycle);
                            std::shared_ptr<AssumptionLiteralExpression> next_negative_assumption_literal =
                                    std::make_shared<AssumptionLiteralExpression>(
                                            _manager->makeBooleanConstant(next_negative_assumption_literal_name));
                            if (z3_evaluated_expression.is_true()) {
                                // relate the next assumption literal to the current assumption literal
                                state.pushAssumptionLiteral(next_positive_assumption_literal_name, assumption_literal);
                                // XXX the effect is pushed into the succeeding block
                                state.pushAssumption(next_positive_assumption_literal_name, encoded_expression);
                                state.setAssumptionLiteral(next_positive_assumption_literal);
                                if (_forked_context.has_value()) {
                                    State &forked_state = (*_forked_context)->getState();
                                    // relate the next assumption literal to the current assumption literal
                                    forked_state.pushAssumptionLiteral(next_negative_assumption_literal_name,
                                                                       assumption_literal);
                                    // XXX the effect is pushed into the succeeding block
                                    forked_state.pushAssumption(next_negative_assumption_literal_name,
                                                                negated_encoded_expression);
                                    forked_state.setAssumptionLiteral(next_negative_assumption_literal);
                                }
                            } else {
                                // relate the next assumption literal to the current assumption literal
                                state.pushAssumptionLiteral(next_negative_assumption_literal_name, assumption_literal);
                                // XXX the effect is pushed into the succeeding block
                                state.pushAssumption(next_negative_assumption_literal_name, negated_encoded_expression);
                                state.setAssumptionLiteral(next_negative_assumption_literal);
                                if (_forked_context.has_value()) {
                                    State &forked_state = (*_forked_context)->getState();
                                    // relate the next assumption literal to the current assumption literal
                                    forked_state.pushAssumptionLiteral(next_positive_assumption_literal_name,
                                                                       assumption_literal);
                                    // XXX the effect is pushed into the succeeding block
                                    forked_state.pushAssumption(next_positive_assumption_literal_name,
                                                                encoded_expression);
                                    forked_state.setAssumptionLiteral(next_positive_assumption_literal);
                                }
                            }
                            break;
                        }
                        case Configuration::BlockEncoding::BASIC:
                            break;
                        default:
                            throw std::runtime_error("Invalid block encoding configured.");
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected encoding mode encountered.");
            }
            break;
        }
        case Configuration::ExecutionMode::SHADOW: {
            _configuration->setShadowProcessingMode(Configuration::ShadowProcessingMode::BOTH);
            std::shared_ptr<Expression> encoded_expression = _encoder->encode(expression, *_context);
            SPDLOG_LOGGER_TRACE(logger, "{}", *encoded_expression);
            // Check if encoded expression contains a nested shadow expression, if yes, evaluate expression under old
            // and new context and compare the outputs
            if (containsShadowExpression(*encoded_expression)) {
                // evaluate old condition
                _configuration->setShadowProcessingMode(Configuration::ShadowProcessingMode::OLD);
                std::shared_ptr<Expression> old_evaluated_expression = _evaluator->evaluate(expression, *_context);
                z3::expr z3_old_evaluated_expression = old_evaluated_expression->getZ3Expression();
                _configuration->setShadowProcessingMode(Configuration::ShadowProcessingMode::NEW);
                std::shared_ptr<Expression> new_evaluated_expression = _evaluator->evaluate(expression, *_context);
                z3::expr z3_new_evaluated_expression = new_evaluated_expression->getZ3Expression();
                assert(z3_old_evaluated_expression.is_bool() && z3_new_evaluated_expression.is_bool());
                assert(z3_old_evaluated_expression.is_true() || z3_old_evaluated_expression.is_false());
                assert(z3_new_evaluated_expression.is_true() || z3_new_evaluated_expression.is_false());

                // i) if divergence is exposed, execution is stopped and context is added for execution during the
                // second phase
                if ((z3_old_evaluated_expression.is_true() && z3_new_evaluated_expression.is_false()) ||
                    (z3_old_evaluated_expression.is_false() && z3_new_evaluated_expression.is_true())) {
                    SPDLOG_LOGGER_TRACE(logger, "Divergent behavior for: {} between old: {} and new: {}",
                                        *encoded_expression, *old_evaluated_expression, *new_evaluated_expression);
                    _execution_status = ExecutionStatus::DIVERGENT_BEHAVIOR;
                    _divergent_contexts = std::vector<std::unique_ptr<Context>>();
                    _divergent_contexts->push_back(std::move(_context));
                } else {
                    // ii) execution follows the same paths in both versions of the program -> check if divergences are
                    // possible
                    _configuration->setShadowProcessingMode(Configuration::ShadowProcessingMode::OLD);
                    std::shared_ptr<Expression> old_encoded_expression = _encoder->encode(expression, *_context);
                    assert(old_encoded_expression->getKind() == Expression::Kind::SYMBOLIC_EXPRESSION);
                    _configuration->setShadowProcessingMode(Configuration::ShadowProcessingMode::NEW);
                    std::shared_ptr<Expression> new_encoded_expression = _encoder->encode(expression, *_context);
                    assert(new_encoded_expression->getKind() == Expression::Kind::SYMBOLIC_EXPRESSION);
                    z3::expr z3_negated_old_encoded_expression = !old_encoded_expression->getZ3Expression();
                    std::shared_ptr<Expression> negated_old_encoded_expression =
                            std::make_shared<SymbolicExpression>(z3_negated_old_encoded_expression);
                    z3::expr z3_negated_new_encoded_expression = !new_encoded_expression->getZ3Expression();
                    std::shared_ptr<Expression> negated_new_encoded_expression =
                            std::make_shared<SymbolicExpression>(z3_negated_new_encoded_expression);

                    // TODO (17.02.2022): For the example Engine_Call_Coverage_In_1_Cycle_With_Change_Shadow_Cfg we
                    //  need to somehow propagate the change shadow expression in the old and new encoded expressions
                    //  as currently it is hidden, i.e., the encoder needs to lower(?) the expression?
                    if (z3_old_evaluated_expression.is_true()) {
                        assert(z3_new_evaluated_expression.is_true());
                        tryDivergentFork(negated_old_encoded_expression, new_encoded_expression);
                    } else {
                        assert(z3_old_evaluated_expression.is_false() && z3_new_evaluated_expression.is_false());
                        tryDivergentFork(old_encoded_expression, negated_new_encoded_expression);
                    }

                    _execution_status = ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR;
                    throw std::logic_error("Not implemented yet.");
                }
            } else {
                // XXX just continue as in the compositional case;
                // XXX CODE DUPLICATION!
                // evaluate condition
                std::shared_ptr<Expression> evaluated_expression = _evaluator->evaluate(expression, *_context);
                z3::expr z3_evaluated_expression = evaluated_expression->getZ3Expression();
                assert(z3_evaluated_expression.is_bool() &&
                       (z3_evaluated_expression.is_true() || z3_evaluated_expression.is_false()));

                // encode condition
                std::shared_ptr<Expression> negated_encoded_expression(nullptr);
                switch (encoded_expression->getKind()) {
                    case Expression::Kind::SYMBOLIC_EXPRESSION: {
                        z3::expr z3_negated_encoded_expression = !encoded_expression->getZ3Expression();
                        negated_encoded_expression =
                                std::make_shared<SymbolicExpression>(z3_negated_encoded_expression);
                        break;
                    }
                    case Expression::Kind::CONCRETE_EXPRESSION:
                    case Expression::Kind::SHADOW_EXPRESSION:
                    default:
                        throw std::logic_error("Not implemented yet.");
                }
                assert(negated_encoded_expression != nullptr);

                const Frame &frame = _context->getFrame();
                const Cfg &cfg = frame.getCfg();
                State &state = _context->getState();
                const Vertex &vertex = state.getVertex();
                unsigned int label = vertex.getLabel();
                const Edge &true_edge = cfg.getTrueEdge(label);
                const Edge &false_edge = cfg.getFalseEdge(label);
                unsigned int next_positive_label = true_edge.getTargetLabel();
                unsigned int next_negative_label = false_edge.getTargetLabel();

                // TODO (05.01.2022): Add assumption expression in tryFork and in these branch cases
                if (z3_evaluated_expression.is_true()) {
                    tryFork(negated_encoded_expression, cfg.getVertex(next_negative_label));
                    std::shared_ptr<Expression> lowered_encoded_expression = _context->lowerExpression(
                            *_manager, encoded_expression, _id_to_uninterpreted_constants, _id_to_lowered_expression);
                    state.pushPathConstraint(lowered_encoded_expression);
                    _context->pushLocalPathConstraint(encoded_expression);
                    state.setVertex(cfg.getVertex(next_positive_label));
                } else {
                    tryFork(encoded_expression, cfg.getVertex(next_positive_label));
                    std::shared_ptr<Expression> lowered_negated_encoded_expression =
                            _context->lowerExpression(*_manager, negated_encoded_expression,
                                                      _id_to_uninterpreted_constants, _id_to_lowered_expression);
                    state.pushPathConstraint(lowered_negated_encoded_expression);
                    _context->pushLocalPathConstraint(negated_encoded_expression);
                    state.setVertex(cfg.getVertex(next_negative_label));
                }
            }
            break;
        }
        default:
            throw std::runtime_error("Invalid execution mode configured.");
    }
}

void Executor::visit(const ir::SequenceInstruction &instruction) {
    for (auto it = instruction.instructionsBegin(); it != instruction.instructionsEnd(); ++it) {
        it->accept(*this);
    }
    throw std::logic_error("Not implemented yet.");
}

// TODO (17.01.2022): See example of SeaHorn: A CHC-based Verification Tool (Slides from Jorge A. Navas, p. 24/49)
void Executor::visit(const ir::WhileInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // evaluate condition
    std::shared_ptr<Expression> evaluated_expression = _evaluator->evaluate(expression, *_context);
    z3::expr z3_evaluated_expression = evaluated_expression->getZ3Expression();
    assert(z3_evaluated_expression.is_bool() &&
           (z3_evaluated_expression.is_true() || z3_evaluated_expression.is_false()));

    // encode condition
    std::shared_ptr<Expression> encoded_expression = _encoder->encode(expression, *_context);
    std::shared_ptr<Expression> negated_encoded_expression(nullptr);
    switch (encoded_expression->getKind()) {
        case Expression::Kind::SYMBOLIC_EXPRESSION: {
            z3::expr z3_negated_encoded_expression = !encoded_expression->getZ3Expression();
            negated_encoded_expression = std::make_shared<SymbolicExpression>(z3_negated_encoded_expression);
            break;
        }
        case Expression::Kind::CONCRETE_EXPRESSION:
        case Expression::Kind::SHADOW_EXPRESSION:
        default:
            throw std::logic_error("Not implemented yet.");
    }
    assert(negated_encoded_expression != nullptr);

    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::HavocInstruction &instruction) {
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = _context->getFlattenedName(name);

    unsigned int cycle = _context->getCycle();
    State &state = _context->getState();
    unsigned int version = _manager->getVersion(flattened_name) + 1;
    _manager->setVersion(flattened_name, version);

    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

    std::shared_ptr<ir::Variable> variable = nullptr;
    if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
        variable = variable_access->getVariable();
    } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
        variable = field_access->getVariableAccess().getVariable();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
    assert(variable != nullptr);
    const auto &data_type = variable->getDataType();
    z3::expr z3_expression = _manager->makeConstant(contextualized_name, data_type);

    // update symbolic expression to unconstrained
    state.setSymbolicExpression(contextualized_name, std::make_shared<SymbolicExpression>(z3_expression));

    // update concrete expression to a random, type-specific value
    state.setConcreteExpression(contextualized_name,
                                std::make_shared<ConcreteExpression>(_manager->makeRandomValue(data_type)));

    // update control-flow
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();

    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);

    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            throw std::logic_error("Not implemented yet.");
            // encode assumption literal
            switch (_configuration->getBlockEncoding()) {
                case Configuration::BlockEncoding::SINGLE: {
                    // TODO (18.01.2022): What about assignment being a merge point? Or a successor of an if-instruction?
                    std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
                    // add the effect of this instruction
                    // state.pushHardConstraint(assumption_literal->getZ3Expression().to_string(),
                    //                         std::make_shared<SymbolicExpression>(lhs_z3_expression == rhs_z3_expression));
                    // relate the next assumption literal to the current assumption literal
                    std::string next_assumption_literal_name =
                            "b_" + frame.getScope() + "_" + std::to_string(next_label) + "__" + std::to_string(cycle);
                    state.pushAssumptionLiteral(next_assumption_literal_name, assumption_literal);
                    z3::expr z3_next_assumption_literal = _manager->makeBooleanConstant(next_assumption_literal_name);
                    std::shared_ptr<AssumptionLiteralExpression> next_assumption_literal =
                            std::make_shared<AssumptionLiteralExpression>(z3_next_assumption_literal);
                    state.setAssumptionLiteral(next_assumption_literal);
                    break;
                }
                case Configuration::BlockEncoding::BASIC:
                    break;
                default:
                    throw std::runtime_error("Invalid block encoding configured.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }
}
