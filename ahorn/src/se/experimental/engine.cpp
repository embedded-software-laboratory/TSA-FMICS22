#include "se/experimental/engine.h"
#include "se/experimental/expression/symbolic_expression.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <set>
#include <sstream>

using namespace se;

Engine::Engine(std::unique_ptr<Configuration> configuration)
    : _configuration(std::move(configuration)), _engine_status(EngineStatus::EXPECTED_BEHAVIOR), _cycle(0),
      _manager(std::make_unique<Manager>()), _explorer(std::make_unique<Explorer>(*_configuration)),
      _executor(std::make_unique<Executor>(*_configuration, *_manager)),
      _merger(std::make_unique<Merger>(*_configuration, *_manager)),
      _test_suite(std::make_unique<TestSuite>(*_manager)) {}

std::ostream &Engine::print(std::ostream &os) const {
    std::stringstream str;
    str << "{\n";
    str << "Explorer: " << *_explorer << ",\n";
    str << "Executor: " << *_executor << ",\n";
    str << "Merger: " << *_merger;
    switch (_configuration->getEngineMode()) {
        case Configuration::EngineMode::COMPOSITIONAL:
            str << ",\n";
            str << "Test Suite: " << *_test_suite;
            break;
        case Configuration::EngineMode::SHADOW:
            break;
        default:
            throw std::runtime_error("Unexpected engine mode encountered.");
    }
    str << "\n";
    str << "}";
    return os << str.str();
}

void Engine::run(const Cfg &cfg) {
    _begin_time_point = std::chrono::system_clock::now();
    throw std::logic_error("Not implemented yet.");
}

void Engine::initialize(const Cfg &cfg) const {
    // initialize the manager
    _manager->initialize(cfg);
    // initialize explorer
    _explorer->initialize(cfg);
    // initialize merger
    _merger->initialize(cfg);
}

std::unique_ptr<Context> Engine::createInitialContext(const Cfg &cfg) const {
    // create initial context
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        std::string contextualized_name = flattened_name + "_" + std::to_string(_manager->getVersion(flattened_name)) +
                                          "__" + std::to_string(_cycle);

        std::shared_ptr<ConcreteExpression> concrete_expression;
        if (it->hasInitialization()) {
            concrete_expression = std::make_shared<ConcreteExpression>(_manager->makeValue(it->getInitialization()));
        } else {
            concrete_expression = std::make_shared<ConcreteExpression>(_manager->makeDefaultValue(it->getDataType()));
        }
        assert(concrete_expression != nullptr);
        contextualized_name_to_concrete_expression.emplace(contextualized_name, concrete_expression);

        // distinguish between "whole-program" input variables and local/output variables
        if (_manager->isWholeProgramInput(flattened_name)) {
            contextualized_name_to_symbolic_expression.emplace(
                    contextualized_name, std::make_shared<SymbolicExpression>(
                                                 _manager->makeConstant(contextualized_name, it->getDataType())));
        } else {
            contextualized_name_to_symbolic_expression.emplace(contextualized_name, concrete_expression);
        }
    }
    std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression;

    std::shared_ptr<AssumptionLiteralExpression> assumption_literal = nullptr;
    std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>, VerificationConditionComparator>
            assumption_literals;
    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
            assumptions;
    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints;
    std::map<std::string, std::string, VerificationConditionComparator> unknown_over_approximating_summary_literals;
    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            // create initial assumption literal
            std::string assumption_literal_name =
                    "b_" + cfg.getName() + "_" + std::to_string(cfg.getEntryLabel()) + "__" + std::to_string(_cycle);
            z3::expr z3_assumption_literal = _manager->makeBooleanConstant(assumption_literal_name);
            assumption_literal = std::make_shared<AssumptionLiteralExpression>(z3_assumption_literal);
            assumption_literals.emplace(
                    assumption_literal_name,
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>>{
                            std::make_shared<AssumptionLiteralExpression>(_manager->makeBooleanValue(true))});
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    auto state = std::make_unique<State>(
            *_configuration, cfg.getVertex(cfg.getEntryLabel()), std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression), std::move(shadow_name_to_shadow_expression),
            std::vector<std::shared_ptr<Expression>>(), std::move(assumption_literal), std::move(assumption_literals),
            std::move(assumptions), std::move(hard_constraints),
            std::move(unknown_over_approximating_summary_literals));
    std::deque<std::shared_ptr<Frame>> frame_stack;
    frame_stack.push_back(std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel()));
    std::unique_ptr<Context> context = std::make_unique<Context>(_cycle, std::move(state), std::move(frame_stack));
    return context;
}

// TODO (25.01.2022): After Kuchta et al. the new version should be used.
std::unique_ptr<Context> Engine::translateDivergentContext(z3::context &z3_context,
                                                           std::unique_ptr<Context> context) const {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Translating divergent context: {}", *context);

    z3::context &z3_seeded_context = _manager->getZ3Context();

    unsigned int cycle = context->getCycle();
    const State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Cfg &cfg = context->getFrame().getCfg();

    std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression;
    // XXX we do not add the shadow store to the translated divergent contexts, as execution should start with the
    // "new" context, however, we need to translate the "new" version to the new context
    for (const auto &shadow_valuation : state.getShadowStore()) {
        std::shared_ptr<ShadowExpression> shadow_expression = shadow_valuation.second;
        z3::expr z3_expression = shadow_expression->getZ3Expression();
        z3::expr z3_seeded_expression =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_expression, z3_seeded_context));
        std::shared_ptr<Expression> old_expression = shadow_expression->getOldExpression();
        z3::expr z3_old_expression = old_expression->getZ3Expression();
        z3::expr z3_seeded_old_expression =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_old_expression, z3_seeded_context));
        std::shared_ptr<Expression> new_expression = shadow_expression->getNewExpression();
        z3::expr z3_new_expression = new_expression->getZ3Expression();
        z3::expr z3_seeded_new_expression =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_new_expression, z3_seeded_context));
        shadow_name_to_shadow_expression.emplace(
                shadow_valuation.first,
                std::make_shared<ShadowExpression>(z3_seeded_expression,
                                                   std::make_shared<SymbolicExpression>(z3_seeded_old_expression),
                                                   std::make_shared<SymbolicExpression>(z3_seeded_new_expression)));
    }

    // TODO (25.01.2022): Replace all shadows with the "new" version in preparation for the execution.
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    for (const auto &concrete_valuation : state.getConcreteStore()) {
        z3::expr z3_expression = concrete_valuation.second->getZ3Expression();
        z3::expr z3_seeded_expression =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_expression, z3_seeded_context));
        z3_seeded_expression = substituteShadowExpression(shadow_name_to_shadow_expression, z3_seeded_expression);
        std::cout << &z3_seeded_expression.ctx() << std::endl;
        contextualized_name_to_concrete_expression.emplace(concrete_valuation.first,
                                                           std::make_shared<ConcreteExpression>(z3_seeded_expression));
    }

    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    for (const auto &symbolic_valuation : state.getSymbolicStore()) {
        z3::expr z3_expression = symbolic_valuation.second->getZ3Expression();
        z3::expr z3_seeded_expression =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_expression, z3_seeded_context));
        z3_seeded_expression = substituteShadowExpression(shadow_name_to_shadow_expression, z3_seeded_expression);
        contextualized_name_to_symbolic_expression.emplace(symbolic_valuation.first,
                                                           std::make_shared<SymbolicExpression>(z3_seeded_expression));
    }

    std::vector<std::shared_ptr<Expression>> path_constraint;
    for (const std::shared_ptr<Expression> &constraint : state.getPathConstraint()) {
        z3::expr z3_constraint = constraint->getZ3Expression();
        z3::expr z3_seeded_constraint =
                z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_constraint, z3_seeded_context));
        z3_seeded_constraint = substituteShadowExpression(shadow_name_to_shadow_expression, z3_seeded_constraint);
        path_constraint.push_back(std::make_shared<SymbolicExpression>(z3_seeded_constraint));
    }

    std::shared_ptr<AssumptionLiteralExpression> seeded_assumption_literal;
    std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>, VerificationConditionComparator>
            seeded_assumption_literals;
    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
            seeded_assumptions;
    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints;
    std::map<std::string, std::string, VerificationConditionComparator> unknown_over_approximating_summary_literals;
    switch (_configuration->getEncodingMode()) {
        case Configuration::EncodingMode::NONE:
            break;
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            // translate assumption literal
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
            z3::expr z3_assumption_literal = assumption_literal->getZ3Expression();

            z3::expr z3_seeded_assumption_literal =
                    z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_assumption_literal, z3_seeded_context));
            seeded_assumption_literal = std::make_shared<AssumptionLiteralExpression>(z3_seeded_assumption_literal);

            // translate assumption literals
            for (const auto &assumption_literals : state.getAssumptionLiterals()) {
                std::vector<std::shared_ptr<AssumptionLiteralExpression>> seeded_ass_lits;
                for (const std::shared_ptr<AssumptionLiteralExpression> &ass_lit : assumption_literals.second) {
                    z3::expr z3_ass_lit_expression = ass_lit->getZ3Expression();
                    z3::expr z3_seeded_ass_lit_expression = z3::to_expr(
                            z3_seeded_context, Z3_translate(z3_context, z3_ass_lit_expression, z3_seeded_context));
                    seeded_ass_lits.push_back(
                            std::make_shared<AssumptionLiteralExpression>(z3_seeded_ass_lit_expression));
                }
                seeded_assumption_literals.emplace(assumption_literals.first, std::move(seeded_ass_lits));
            }

            // translate assumptions
            for (const auto &assumption : state.getAssumptions()) {
                std::vector<std::shared_ptr<SymbolicExpression>> seeded_assumption_expressions;
                for (const std::shared_ptr<SymbolicExpression> &assumption_expression : assumption.second) {
                    z3::expr z3_assumption_expression = assumption_expression->getZ3Expression();
                    z3::expr z3_seeded_assumption_expression = z3::to_expr(
                            z3_seeded_context, Z3_translate(z3_context, z3_assumption_expression, z3_seeded_context));
                    z3_seeded_assumption_expression = substituteShadowExpression(shadow_name_to_shadow_expression,
                                                                                 z3_seeded_assumption_expression);
                    seeded_assumption_expressions.push_back(
                            std::make_shared<SymbolicExpression>(z3_seeded_assumption_expression));
                }
                seeded_assumptions.emplace(assumption.first, std::move(seeded_assumption_expressions));
            }

            // translate hard constraints
            for (const auto &hard_constraint : state.getHardConstraints()) {
                std::map<std::string, z3::expr> seeded_hard_constraints;
                for (const auto &hard_constraint_expression_pair : hard_constraint.second) {
                    z3::expr z3_hard_constraint_expression = hard_constraint_expression_pair.second;
                    z3::expr z3_seeded_hard_constraint_expression =
                            z3::to_expr(z3_seeded_context,
                                        Z3_translate(z3_context, z3_hard_constraint_expression, z3_seeded_context));
                    z3_seeded_hard_constraint_expression = substituteShadowExpression(
                            shadow_name_to_shadow_expression, z3_seeded_hard_constraint_expression);
                    seeded_hard_constraints.emplace(hard_constraint_expression_pair.first,
                                                    z3_seeded_hard_constraint_expression);
                }
                hard_constraints.emplace(hard_constraint.first, std::move(seeded_hard_constraints));
            }

            unknown_over_approximating_summary_literals = state.getUnknownOverApproximatingSummaryLiterals();
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    // XXX ignore shadow store
    shadow_name_to_shadow_expression.clear();
    std::unique_ptr<State> seeded_state = std::make_unique<State>(
            *_configuration, cfg.getVertex(label), std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression), std::move(shadow_name_to_shadow_expression),
            std::move(path_constraint), std::move(seeded_assumption_literal), std::move(seeded_assumption_literals),
            std::move(seeded_assumptions), std::move(hard_constraints),
            std::move(unknown_over_approximating_summary_literals));

    std::deque<std::shared_ptr<Frame>> frame_stack;
    for (const std::shared_ptr<Frame> &frame : context->getFrameStack()) {
        std::shared_ptr<Frame> seeded_frame =
                std::make_shared<Frame>(frame->getCfg(), frame->getScope(), frame->getLabel());
        for (const std::shared_ptr<Expression> &local_constraint : frame->getLocalPathConstraint()) {
            z3::expr z3_local_constraint = local_constraint->getZ3Expression();
            z3::expr z3_seeded_local_constraint =
                    z3::to_expr(z3_seeded_context, Z3_translate(z3_context, z3_local_constraint, z3_seeded_context));
            seeded_frame->pushLocalPathConstraint(std::make_shared<SymbolicExpression>(z3_seeded_local_constraint));
        }
        frame_stack.push_back(std::move(seeded_frame));
    }
    std::unique_ptr<Context> seeded_context =
            std::make_unique<Context>(cycle, std::move(seeded_state), std::move(frame_stack));

    SPDLOG_LOGGER_TRACE(logger, "Resulting seeded context: {}", *seeded_context);

    return seeded_context;
}

// Substitutes shadow expressions with the expression corresponding to the new version
z3::expr Engine::substituteShadowExpression(
        const std::map<std::string, std::shared_ptr<ShadowExpression>> &shadow_name_to_shadow_expression,
        const z3::expr &z3_expression) const {
    auto logger = spdlog::get("Symbolic Execution");

    z3::expr z3_substituted_expression = z3_expression.simplify();
    std::vector<z3::expr> uninterpreted_constants = _manager->getUninterpretedConstantsFromZ3Expression(z3_expression);
    for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
        std::string contextualized_name = uninterpreted_constant.decl().name().str();
        std::size_t shadow_position = contextualized_name.find("shadow");
        if (shadow_position != std::string::npos) {
            std::shared_ptr<ShadowExpression> shadow_expression =
                    shadow_name_to_shadow_expression.at(contextualized_name);
            std::shared_ptr<Expression> new_expression = shadow_expression->getNewExpression();
            z3::expr z3_new_expression = new_expression->getZ3Expression();
            SPDLOG_LOGGER_TRACE(logger, "Found shadow expression, replacing expression: {} with new expression: {}",
                                uninterpreted_constant.to_string(), z3_new_expression.to_string());
            z3_substituted_expression =
                    _manager->substituteZ3Expression(z3_expression, uninterpreted_constant, z3_new_expression)
                            .simplify();
        }
    }

    return z3_substituted_expression.simplify();
}

std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> Engine::step() {
    auto logger = spdlog::get("Symbolic Execution");

    EngineStatus engine_status = EngineStatus::EXPECTED_BEHAVIOR;
    std::vector<std::unique_ptr<Context>> contexts;

    switch (_configuration->getStepSize()) {
        case Configuration::StepSize::INSTRUCTION: {
            throw std::logic_error("Not implemented yet.");
        }
        case Configuration::StepSize::CYCLE: {
            while (!_explorer->isEmpty() || !_merger->isEmpty()) {
                SPDLOG_LOGGER_TRACE(logger, "{}", *_explorer);
                SPDLOG_LOGGER_TRACE(logger, "{}", *_merger);
                // merge strategy can not be implemented in the explorer as it can not execute contexts, hence
                // the engine manages the "communication" between the explorer, executor, and the merger. In case
                // there are no contexts in the explorer queue anymore, we close merge points. We can safely assume
                // that the merge point can be closed, as we prioritize the merging of "lower" merge points before
                // merging "higher" merge points.
                // Note: the invariant,  that all prior subpaths have been discovered already before reaching the
                // potential merge point is only valid if we use breadth-first search in "topological" order
                // (the order is actually determined by the compiler of our ST parser). In case we use depth-first
                // search, this invariant does not hold anymore, and hence this will block. We can not "close" the
                // merge point, if there exist contexts with a lower label in the queue.
                // Hence we use a map from merge point to mergeable context queue. In case we have bfs heuristic,
                // this is not necessary, but in case we have dfs, and merge point is set to all join points, we
                // change the execution order and let dfs catch-up at join points. As different paths can be taken,
                // we should not block. Once the explorer queue is empty, one has to check whether the merger has
                // "open" merge points -> one (all?) of them must be closed and the exploration must continue with the
                // merged context.
                if (_explorer->isEmpty()) {
                    // TODO (27.01.2022): The problem is, that we do not prioritize states from shallower contexts,
                    //  i.e., currently the deepest contexts are prioritized -> change this behavior!
                    SPDLOG_LOGGER_TRACE(logger, "No more contexts for exploration exist, falling back on merge.");
                    assert(!_merger->isEmpty());

                    // 1. get most deep merge point
                    // 2. check if there are any other contexts waiting at merge points that reach the most deep
                    // merge point (recurse, obviously)
                    // 3. prioritize the most shallow context residing at a merge point at which he can progress,
                    // else take the most deep merge point
                    std::chrono::time_point<std::chrono::system_clock> begin_time_point =
                            std::chrono::system_clock::now();
                    std::unique_ptr<Context> merged_context = _merger->merge();
                    using namespace std::literals;
                    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
                    std::cout << "Elapsed time (merging): " << elapsed_time << "ms" << std::endl;
                    _explorer->pushContext(std::move(merged_context));
                } else {
                    std::unique_ptr<Context> context = _explorer->popContext();
                    const State &state = context->getState();
                    const Vertex &vertex = state.getVertex();
                    unsigned int label = vertex.getLabel();

                    std::pair<Executor::ExecutionStatus, std::vector<std::unique_ptr<Context>>> execution_result =
                            _executor->execute(std::move(context));

                    switch (_configuration->getEngineMode()) {
                        case Configuration::EngineMode::COMPOSITIONAL: {
                            assert(execution_result.first == Executor::ExecutionStatus::EXPECTED_BEHAVIOR);
                            engine_status = EngineStatus::EXPECTED_BEHAVIOR;
                            for (std::unique_ptr<Context> &succeeding_context : execution_result.second) {
                                // TODO (12.01.2022): Use coverage values for heuristic of explorer?
                                // TODO (12.01.2022): Check if coverage has changed, if yes, then add the context to the test
                                //  suite such we can derive a test case.
                                std::pair<bool, bool> coverage = _explorer->updateCoverage(label, *succeeding_context);
                                if (coverage.second) {
                                    SPDLOG_LOGGER_TRACE(logger,
                                                        "Branch coverage has been increased, creating test case...");
                                    _test_suite->createTestCase(*succeeding_context);
                                }
                                // TODO (03.01.2022): If the coverage has changed, append/create a test case using test suite?
                                //  But only, if we are going into the next cycle -> (12.01.2022): only at cycle end looses
                                //  information
                                if (_merger->reachedMergePoint(*succeeding_context)) {
                                    SPDLOG_LOGGER_TRACE(logger, "Reached merge point: {}",
                                                        succeeding_context->getState().getVertex().getLabel());
                                    _merger->pushContext(std::move(succeeding_context));
                                } else if (_cycle < succeeding_context->getCycle()) {
                                    contexts.push_back(std::move(succeeding_context));
                                } else {
                                    _explorer->pushContext(std::move(succeeding_context));
                                }
                            }
                            break;
                        }
                        case Configuration::EngineMode::SHADOW: {
                            switch (execution_result.first) {
                                case Executor::ExecutionStatus::EXPECTED_BEHAVIOR: {
                                    // XXX expected behavior during SSE renders coverage and test case generation
                                    // obsolete, as execution is seeded with inputs from a test case
                                    engine_status = EngineStatus::EXPECTED_BEHAVIOR;
                                    for (std::unique_ptr<Context> &succeeding_context : execution_result.second) {
                                        if (_merger->reachedMergePoint(*succeeding_context)) {
                                            SPDLOG_LOGGER_TRACE(logger, "Reached merge point: {}",
                                                                succeeding_context->getState().getVertex().getLabel());
                                            _merger->pushContext(std::move(succeeding_context));
                                        } else if (_cycle < succeeding_context->getCycle()) {
                                            contexts.push_back(std::move(succeeding_context));
                                        } else {
                                            _explorer->pushContext(std::move(succeeding_context));
                                        }
                                    }
                                    break;
                                }
                                case Executor::ExecutionStatus::DIVERGENT_BEHAVIOR: {
                                    engine_status = EngineStatus::DIVERGENT_BEHAVIOR;
                                    for (std::unique_ptr<Context> &divergent_context : execution_result.second) {
                                        contexts.push_back(std::move(divergent_context));
                                    }
                                    break;
                                }
                                case Executor::ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR:
                                    throw std::logic_error("Not implemented yet.");
                                default:
                                    throw std::runtime_error("Unexpected execution status encountered.");
                            }
                            break;
                        }
                        default:
                            throw std::runtime_error("Invalid engine mode configured.");
                    }
                }
            }
            _cycle = _cycle + 1;
            break;
        }
        default:
            throw std::runtime_error("Invalid step size configured.");
    }

    return std::make_pair(engine_status, std::move(contexts));
}