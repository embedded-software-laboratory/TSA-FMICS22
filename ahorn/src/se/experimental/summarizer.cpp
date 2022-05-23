#include "se/experimental/summarizer.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <sstream>

using namespace se;

Summarizer::Summarizer(Manager &manager)
    : _manager(&manager),
      _type_representative_name_to_summary(std::map<std::string, std::vector<std::unique_ptr<Summary>>>()) {}

std::ostream &Summarizer::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tsize: " << _type_representative_name_to_summary.size() << "\n";
    str << ")";
    return os << str.str();
}

// TODO (21.01.2022): Return a pair, std::pair<bool, result>, where bool denotes whether an applicable summary has
//  been found or not, and the result is boost::optional and holds the valuations. What about changes to symbolic
//  variables? Can they occur?
void Summarizer::findApplicableSummary(const Context &context) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Trying to find an applicable summary...");

    unsigned int cycle = context.getCycle();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();
    std::string type_representative_name = cfg.getName();
    const State &state = context.getState();

    auto it = _type_representative_name_to_summary.find(type_representative_name);
    if (it != _type_representative_name_to_summary.end()) {
        // 1. get the current context, i.e., the valuations of the variables at the entry of the function block
        const ir::Interface &interface = cfg.getInterface();
        std::map<std::string, std::shared_ptr<ConcreteExpression>> reversioned_names_to_concrete_expression;
        for (auto variable = interface.variablesBegin(); variable != interface.variablesEnd(); ++variable) {
            std::string flattened_name = context.getFlattenedName(variable->getName());
            unsigned int version = state.getMaximumVersionFromConcreteStore(flattened_name, cycle);
            std::string contextualized_name =
                    flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
            std::shared_ptr<Expression> expression = state.getConcreteExpression(contextualized_name);
            std::shared_ptr<ConcreteExpression> concrete_expression =
                    std::dynamic_pointer_cast<ConcreteExpression>(expression);
            assert(concrete_expression != nullptr);
            std::string reversioned_name = flattened_name + "_" + std::to_string(0);
            SPDLOG_LOGGER_TRACE(logger, "{} reversioned to {} -> {}", contextualized_name, reversioned_name,
                                *concrete_expression);
            reversioned_names_to_concrete_expression.emplace(std::move(reversioned_name),
                                                             std::move(concrete_expression));
        }
        bool found_applicable_summary = false;
        // TODO (20.01.2022): Summary pruning idea: retrieve the unsat core of an unapplicable summary and discard
        //  all other summaries that share the same assumption literals. We could, e.g., ask the solver step by step
        //  if the path described by the assumption literals is satisfiable, once it is unsatisfiable, we stop and
        //  this is the reason why its not satisfiable
        for (const auto &summary : it->second) {
            SPDLOG_LOGGER_TRACE(logger, "Trying to apply: {}", *summary);
            // TODO (21.01.2022): Due to merging we currently always use the concrete valuations from one of the two
            //  merged contexts. This also biases our summary applicability, because these are dependent on the
            //  concrete valuations.
            if (isSummaryApplicable(*summary, reversioned_names_to_concrete_expression)) {
                SPDLOG_LOGGER_TRACE(logger, "Summary is applicable!");
            } else {
                SPDLOG_LOGGER_TRACE(logger, "Summary is not applicable!");
            }
            // TODO (20.01.2022): Version alignment of variables
        }
    } else {
        SPDLOG_LOGGER_TRACE(logger, "No summary for {} exists.", type_representative_name);
    }
}

void Summarizer::summarize(const Context &context) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Building summaries...");

    unsigned int cycle = context.getCycle();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    std::string scope = frame.getScope();

    const State &state = context.getState();

    // Get all paths that are contained in the context. If summarization occurs at FunctionBlockExit, but the contexts
    // reaching this exit are already merged, one has to go back from the merge point and consider all prior
    // assumption literals and collect them to build all "feasible" paths.
    // TODO (20.01.2022): Can we retrieve infeasible paths this way? I think not, because symbolic execution
    //  guarantees that only feasible assumption literals are generated during verification condition generation.
    std::vector<std::vector<std::shared_ptr<AssumptionLiteralExpression>>> paths =
            determineAssumptionLiteralPaths(context);

    // TODO (21.01.2022): Currently, multiple summaries for the same paths are generated. This will properly be
    //  resolved in case we actually apply the summary and hence skip the generation (for the redundant summary).
    //  Nevertheless, shall we implement a quick check here?
    std::vector<std::unique_ptr<Summary>> summaries;
    summaries.reserve(paths.size());
    for (const auto &path : paths) {
        summaries.emplace_back(summarizePath(context, path));
    }

    // Expected preconditions and postconditions for the example: Engine_Call_Cfg
    // Pre:
    // control-flow: b_P.f_0__0 -> true, b_P.f_2__0 -> b_P.f_0__0, b_P.f_6__0 -> b_P.f_2__0
    // assumptions: b_P.f_2__0 -> (>= P.f.a_1__0 32)
    // Post:
    // hard constraints: b_P.f_2__0 -> (= P.f.b_2__0 64), (= P.f.b_3__0 P.f.b_2__0)

    // TODO (19.01.2022): Decontextualized the names, but keep the versioning somehow intact.
    // TODO (20.01.2022): Re-version to start at 0
    // TODO (19.01.2022): Build pre/post conditions
    // Im grunde kann ich doch sagen, dass alles was VOR den locations wo die assumptions abgefragt werden zur ENTRY
    // condition gehören und alles was danach kommt einfach zur POST condition (ist ja der effect)

    SPDLOG_LOGGER_TRACE(logger, "Generated {} summaries.", summaries.size());
    for (const auto &summary : summaries) {
        SPDLOG_LOGGER_TRACE(logger, "Generated summary: {}", *summary);
    }
    std::string type_representative_name = cfg.getFullyQualifiedName();
    auto it = _type_representative_name_to_summary.find(type_representative_name);
    if (it == _type_representative_name_to_summary.end()) {
        _type_representative_name_to_summary.emplace(type_representative_name, std::move(summaries));
    } else {
        for (std::unique_ptr<Summary> &summary : summaries) {
            it->second.push_back(std::move(summary));
        }
    }
}

// TODO (21.01.2022): In case the summary is not applicable (unsat), this function should return the core such that
//  we can prune other summaries containing the same assumption literal / decision.
bool Summarizer::isSummaryApplicable(
        const Summary &summary,
        const std::map<std::string, std::shared_ptr<ConcreteExpression>> &reversioned_names_to_concrete_expression) {
    auto logger = spdlog::get("Symbolic Execution");

    z3::context &z3_context = _manager->getZ3Context();
    z3::tactic tactic =
            z3::tactic(z3_context, "simplify") & z3::tactic(z3_context, "solve-eqs") & z3::tactic(z3_context, "smt");
    z3::solver z3_solver = tactic.mk_solver();
    z3_solver.set("unsat_core", true);

    // constraints from the context under which the callee is called
    z3::expr_vector context_constraints(z3_context);
    for (const auto &reversioned_name_to_concrete_expression : reversioned_names_to_concrete_expression) {
        std::string reversioned_name = reversioned_name_to_concrete_expression.first;
        z3::expr z3_concrete_expression = reversioned_name_to_concrete_expression.second->getZ3Expression();
        if (z3_concrete_expression.is_bool()) {
            context_constraints.push_back(_manager->makeBooleanConstant(reversioned_name) == z3_concrete_expression);
        } else if (z3_concrete_expression.is_int()) {
            context_constraints.push_back(_manager->makeIntegerConstant(reversioned_name) == z3_concrete_expression);
        } else {
            throw std::runtime_error("Unsupported z3::expr sort.");
        }
    }

    // control-flow constraints
    z3::expr_vector control_flow_constraints(z3_context);
    for (const auto &assumption_literal : summary.getAssumptionLiterals()) {
        std::string assumption_literal_name = assumption_literal.first;
        z3::expr z3_preceding_assumption_literal_expression = assumption_literal.second->getZ3Expression();
        control_flow_constraints.push_back(z3::implies(_manager->makeBooleanConstant(assumption_literal_name),
                                                       z3_preceding_assumption_literal_expression));
        z3::expr_vector expressions(z3_context);
        expressions.push_back(z3_preceding_assumption_literal_expression);
    }

    // assumption constraints
    z3::expr_vector assumption_constraints(z3_context);
    for (const auto &assumption : summary.getAssumptions()) {
        std::string assumption_literal_name = assumption.first;
        z3::expr_vector assumption_expressions(z3_context);
        for (const auto &assumption_expression : assumption.second) {
            assumption_expressions.push_back(assumption_expression->getZ3Expression());
        }
        assumption_constraints.push_back(z3::implies(_manager->makeBooleanConstant(assumption_literal_name),
                                                     z3::mk_and(assumption_expressions)));
    }

    // hard constraints
    z3::expr_vector hard_constraints(z3_context);
    for (const auto &hard_constraint : summary.getHardConstraints()) {
        std::string assumption_literal_name = hard_constraint.first;
        z3::expr_vector hard_constraint_expressions(z3_context);
        for (const auto &hard_constraint_expression_pair : hard_constraint.second) {
            if (hard_constraint_expression_pair.second.is_bool()) {
                hard_constraint_expressions.push_back(
                        _manager->makeBooleanConstant(hard_constraint_expression_pair.first) ==
                        hard_constraint_expression_pair.second);
            } else if (hard_constraint_expression_pair.second.is_int()) {
                hard_constraint_expressions.push_back(
                        _manager->makeIntegerConstant(hard_constraint_expression_pair.first) ==
                        hard_constraint_expression_pair.second);
            } else {
                throw std::runtime_error("Unsupported z3::sort encountered.");
            }
        }
        hard_constraints.push_back(z3::implies(_manager->makeBooleanConstant(assumption_literal_name),
                                               z3::mk_and(hard_constraint_expressions)));
    }

    // TODO (20.01.2022): Should the context constraints be implied by the first assumption literal? -> Dimitri
    //  simply adds it to the solver.
    z3_solver.add(context_constraints);
    z3_solver.add(control_flow_constraints);
    z3_solver.add(assumption_constraints);
    z3_solver.add(hard_constraints);

    SPDLOG_LOGGER_TRACE(logger, "Z3 Solver assertions: {}", z3_solver.assertions());

    // XXX Track assertions and try to retrieve a fine-grained core, if unsat (c.f.
    // https://github.com/Z3Prover/z3/blob/c00591daaff8258a80e9415fc94b4d8869a650a9/examples/c%2B%2B/example.cpp#L430)
    std::vector<z3::expr> assumptions;
    for (const auto &assumption_literal : summary.getAssumptionLiterals()) {
        assumptions.push_back(_manager->makeBooleanConstant(assumption_literal.first));
    }
    z3::check_result z3_result = z3_solver.check(assumptions.size(), &assumptions[0]);

    switch (z3_result) {
        case z3::unsat: {
            SPDLOG_LOGGER_TRACE(logger, "solver returned unsat, summary is not applicable.");
            z3::expr_vector unsat_core = z3_solver.unsat_core();
            SPDLOG_LOGGER_TRACE(logger, "unsat core: {}", unsat_core);
            std::cout << "size: " << unsat_core.size() << "\n";
            for (unsigned i = 0; i < unsat_core.size(); i++) {
                std::cout << unsat_core[i] << "\n";
            }
            break;
        }
        case z3::sat: {
            z3::model z3_model = z3_solver.get_model();
            SPDLOG_LOGGER_TRACE(logger, "solver returned sat, summary is applicable.");
            std::stringstream str;
            for (unsigned int i = 0; i < z3_model.size(); ++i) {
                z3::func_decl constant_declaration = z3_model.get_const_decl(i);
                assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
                std::string contextualized_name = constant_declaration.name().str();
                z3::expr interpretation = z3_model.get_const_interp(constant_declaration);
                str << contextualized_name << " -> " << interpretation << "\n";
            }
            SPDLOG_LOGGER_TRACE(logger, "model:\n{}", str.str());
            break;
        }
        case z3::unknown:
            throw std::runtime_error("Unknown z3::check_result.");
        default:
            throw std::runtime_error("Invalid z3::check_result.");
    }

    return z3_result == z3::sat;
}

// TODO (19.01.2022): If we try to rebuild the path using the assumption literals, we will also encode possible
//  infeasible paths. This does not make us incomplete, but it is an unnecessary over-approximation. We already
//  traverse each path, so we can also write down which paths we have taken. -> I don't think so, because we only
//  encode the feasible PATHS during symbolic execution (based on the current path constraint).
std::vector<std::vector<std::shared_ptr<AssumptionLiteralExpression>>>
Summarizer::determineAssumptionLiteralPaths(const Context &context) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Determining assumption literal paths for summarization...");

    unsigned int cycle = context.getCycle();
    const State &state = context.getState();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    std::shared_ptr<AssumptionLiteralExpression> assumption_literal = state.getAssumptionLiteral();
    std::string assumption_literal_name = assumption_literal->getZ3Expression().to_string();

    std::string entry_assumption_literal_name =
            "b_" + frame.getScope() + "_" + std::to_string(cfg.getEntryLabel()) + "__" + std::to_string(cycle);

    std::vector<std::vector<std::shared_ptr<AssumptionLiteralExpression>>> open_paths;
    std::vector<std::vector<std::shared_ptr<AssumptionLiteralExpression>>> closed_paths;

    // initialize the open paths
    for (const auto &preceding_assumption_literal : state.getAssumptionLiterals(assumption_literal_name)) {
        open_paths.emplace_back(std::vector<std::shared_ptr<AssumptionLiteralExpression>>{
                assumption_literal, preceding_assumption_literal});
    }

    while (!open_paths.empty()) {
        std::vector<std::shared_ptr<AssumptionLiteralExpression>> current_path = open_paths.back();
        open_paths.pop_back();
        std::string current_assumption_literal_name = current_path.back()->getZ3Expression().to_string();
        bool should_close_path = true;
        while (current_assumption_literal_name != entry_assumption_literal_name) {
            std::vector<std::shared_ptr<AssumptionLiteralExpression>> preceding_assumption_literals =
                    state.getAssumptionLiterals(current_assumption_literal_name);
            if (preceding_assumption_literals.size() == 1) {
                current_path.emplace_back(preceding_assumption_literals.at(0));
                current_assumption_literal_name = preceding_assumption_literals.at(0)->getZ3Expression().to_string();
            } else {
                for (const auto &preceding_assumption_literal : preceding_assumption_literals) {
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>> new_path = current_path;
                    new_path.emplace_back(preceding_assumption_literal);
                    open_paths.emplace_back(std::move(new_path));
                }
                should_close_path = false;
                break;
            }
        }
        if (should_close_path) {
            closed_paths.emplace_back(current_path);
        }
    }

    // DEBUG
    std::stringstream str;
    int path_count = 0;
    for (const auto &closed_path : closed_paths) {
        str << path_count << ": [";
        for (auto it = closed_path.begin(); it != closed_path.end(); ++it) {
            str << **it;
            if (std::next(it) != closed_path.end()) {
                str << ", ";
            }
        }
        str << "]\n";
        path_count++;
    }
    assert(!closed_paths.empty());
    SPDLOG_LOGGER_TRACE(logger, "Paths to summarize:\n{}", str.str());

    return closed_paths;
}

std::unique_ptr<Summary>
Summarizer::summarizePath(const Context &context,
                          const std::vector<std::shared_ptr<AssumptionLiteralExpression>> &path) const {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Summarizing path...");

    unsigned int cycle = context.getCycle();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator>
            assumption_literals;
    for (auto it = path.begin(); it != path.end(); ++it) {
        std::string assumption_literal_name = (*it)->getZ3Expression().to_string();
        if (std::next(it) != path.end()) {
            std::string next_assumption_literal_name = (*(std::next(it)))->getZ3Expression().to_string();
            assumption_literals.emplace(extractUncycledName(assumption_literal_name),
                                        std::make_shared<AssumptionLiteralExpression>(_manager->makeBooleanConstant(
                                                extractUncycledName(next_assumption_literal_name))));
        } else {
            assumption_literals.emplace(
                    extractUncycledName(assumption_literal_name),
                    std::make_shared<AssumptionLiteralExpression>(_manager->makeBooleanValue(true)));
        }
    }

    // "Re-versioning" is done in two passes. The first pass collects the corresponding contextualized names and
    // reversions them. The second pass substitutes the z3 expressions and builds the data structure for the summary
    // construction.

    // Pass 1: collect contextualized names and reversion them, distinguish between contextualized names that were
    // written and contextualized names that were read, hence we collect the modified contextualized names additionally.

    // Order is important, because "re-version" variables based on their contextualized names. P.f.b_11 is sorted
    // before P.f.b_2 using the default comparator, hence, a custom comparator is introduced to always ensure
    // ascending order of versions.
    auto version_comparer = [](const std::string &contextualized_name_1, const std::string &contextualized_name_2) {
        std::size_t version_position_1 = contextualized_name_1.find('_');
        assert(version_position_1 != std::string::npos);
        std::size_t version_position_2 = contextualized_name_2.find('_');
        assert(version_position_2 != std::string::npos);
        std::string name_1 = contextualized_name_1.substr(0, version_position_1);
        std::string name_2 = contextualized_name_2.substr(0, version_position_2);
        std::size_t cycle_position_1 = contextualized_name_1.find("__");
        assert(cycle_position_1 != std::string::npos);
        std::size_t cycle_position_2 = contextualized_name_2.find("__");
        assert(cycle_position_2 != std::string::npos);
        unsigned int version_1 = std::stoi(contextualized_name_1.substr(version_position_1 + 1, cycle_position_1));
        unsigned int version_2 = std::stoi(contextualized_name_2.substr(version_position_2 + 1, cycle_position_2));
        unsigned int cycle_1 =
                std::stoi(contextualized_name_1.substr(cycle_position_1 + 2, contextualized_name_1.size()));
        unsigned int cycle_2 =
                std::stoi(contextualized_name_2.substr(cycle_position_2 + 2, contextualized_name_2.size()));
        if (name_1 < name_2) {
            return true;
        } else if (name_1 == name_2) {
            if (cycle_1 == cycle_2) {
                if (version_1 < version_2) {
                    return true;
                } else {
                    return false;
                }
            } else {
                throw std::runtime_error("Both contextualized names must have the same cycle during summarization.");
            }
        } else {
            return false;
        }
    };

    std::map<std::string, std::set<std::string, decltype(version_comparer)>> flattened_names_to_contextualized_names;
    std::set<std::string, decltype(version_comparer)> modified_contextualized_names(version_comparer);
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.variablesBegin(); it != interface.variablesEnd(); ++it) {
        std::string flattened_name = context.getFlattenedName(it->getName());
        flattened_names_to_contextualized_names.emplace(
                flattened_name, std::set<std::string, decltype(version_comparer)>(version_comparer));
    }

    const State &state = context.getState();
    for (const auto &assumption_literal : path) {
        std::string assumption_literal_name = assumption_literal->getZ3Expression().to_string();
        auto assumption_it = std::find_if(state.getAssumptions().begin(), state.getAssumptions().end(),
                                          [assumption_literal_name](const auto &assumption) {
                                              return assumption_literal_name == assumption.first;
                                          });
        if (assumption_it != state.getAssumptions().end()) {
            for (const auto &assumption_expression : assumption_it->second) {
                z3::expr z3_assumption_expression = assumption_expression->getZ3Expression();
                for (const auto &uninterpreted_constant :
                     _manager->getUninterpretedConstantsFromZ3Expression(z3_assumption_expression)) {
                    std::string contextualized_name = uninterpreted_constant.to_string();
                    std::string flattened_name = extractFlattenedName(contextualized_name);
                    flattened_names_to_contextualized_names.at(flattened_name).insert(contextualized_name);
                }
            }
        }
        auto hard_constraint_it = std::find_if(state.getHardConstraints().begin(), state.getHardConstraints().end(),
                                               [assumption_literal_name](const auto &hard_constraint) {
                                                   return assumption_literal_name == hard_constraint.first;
                                               });
        if (hard_constraint_it != state.getHardConstraints().end()) {
            for (const auto &hard_constraint_pair : hard_constraint_it->second) {
                z3::expr z3_hard_constraint_expression = hard_constraint_pair.second;
                {
                    std::string contextualized_name = hard_constraint_pair.first;
                    modified_contextualized_names.insert(contextualized_name);
                    std::string flattened_name = extractFlattenedName(contextualized_name);
                    flattened_names_to_contextualized_names.at(flattened_name).insert(contextualized_name);
                }
                for (const auto &uninterpreted_constant :
                     _manager->getUninterpretedConstantsFromZ3Expression(z3_hard_constraint_expression)) {
                    std::string contextualized_name = uninterpreted_constant.to_string();
                    std::string flattened_name = extractFlattenedName(contextualized_name);
                    flattened_names_to_contextualized_names.at(flattened_name).insert(contextualized_name);
                }
            }
        }
    }

    std::map<std::string, std::string, decltype(version_comparer)> contextualized_names_to_reversioned_names(
            version_comparer);
    for (const auto &flattened_name_to_contextualized_name : flattened_names_to_contextualized_names) {
        unsigned int version = 0;
        for (const auto &contextualized_name : flattened_name_to_contextualized_name.second) {
            auto it = modified_contextualized_names.find(contextualized_name);
            if (it != modified_contextualized_names.end() && version == 0) {
                version = 1;
                std::string reversioned_name =
                        flattened_name_to_contextualized_name.first + "_" + std::to_string(version);
                contextualized_names_to_reversioned_names.emplace(contextualized_name, reversioned_name);
                version = version + 1;
            } else {
                std::string reversioned_name =
                        flattened_name_to_contextualized_name.first + "_" + std::to_string(version);
                contextualized_names_to_reversioned_names.emplace(contextualized_name, reversioned_name);
                version = version + 1;
            }
        }
    }

    // Pass 2: reversion contextualized names across the assumptions and hard constraints and collect them for the
    // construction of the summary
    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
            assumptions;
    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints;
    for (const auto &assumption_literal : path) {
        std::string assumption_literal_name = assumption_literal->getZ3Expression().to_string();
        auto assumption_it = std::find_if(state.getAssumptions().begin(), state.getAssumptions().end(),
                                          [assumption_literal_name](const auto &assumption) {
                                              return assumption_literal_name == assumption.first;
                                          });
        if (assumption_it != state.getAssumptions().end()) {
            std::vector<std::shared_ptr<SymbolicExpression>> assumption_expressions;
            for (const auto &assumption_expression : assumption_it->second) {
                z3::expr z3_assumption_expression = assumption_expression->getZ3Expression();
                for (const auto &uninterpreted_constant :
                     _manager->getUninterpretedConstantsFromZ3Expression(z3_assumption_expression)) {
                    std::string contextualized_name = uninterpreted_constant.to_string();
                    std::string reversioned_name = contextualized_names_to_reversioned_names.at(contextualized_name);
                    if (uninterpreted_constant.is_bool()) {
                        z3::expr z3_reversioned_uninterpreted_constant =
                                _manager->makeBooleanConstant(reversioned_name);
                        z3_assumption_expression =
                                _manager->substituteZ3Expression(z3_assumption_expression, uninterpreted_constant,
                                                                 z3_reversioned_uninterpreted_constant);
                    } else if (uninterpreted_constant.is_int()) {
                        z3::expr z3_reversioned_uninterpreted_constant =
                                _manager->makeIntegerConstant(reversioned_name);
                        z3_assumption_expression =
                                _manager->substituteZ3Expression(z3_assumption_expression, uninterpreted_constant,
                                                                 z3_reversioned_uninterpreted_constant);
                    } else {
                        throw std::runtime_error("Unsupported z3::expr sort.");
                    }
                }
                assumption_expressions.push_back(std::make_shared<SymbolicExpression>(z3_assumption_expression));
            }
            assumptions.emplace(extractUncycledName(assumption_literal_name), std::move(assumption_expressions));
        }
        auto hard_constraint_it = std::find_if(state.getHardConstraints().begin(), state.getHardConstraints().end(),
                                               [assumption_literal_name](const auto &hard_constraint) {
                                                   return assumption_literal_name == hard_constraint.first;
                                               });
        if (hard_constraint_it != state.getHardConstraints().end()) {
            std::map<std::string, z3::expr> hard_constraint_expressions;
            for (const auto &hard_constraint_pair : hard_constraint_it->second) {
                z3::expr z3_hard_constraint_expression = hard_constraint_pair.second;
                for (const auto &uninterpreted_constant :
                     _manager->getUninterpretedConstantsFromZ3Expression(z3_hard_constraint_expression)) {
                    std::string contextualized_name = uninterpreted_constant.to_string();
                    std::string reversioned_name = contextualized_names_to_reversioned_names.at(contextualized_name);
                    if (uninterpreted_constant.is_bool()) {
                        z3::expr z3_reversioned_uninterpreted_constant =
                                _manager->makeBooleanConstant(reversioned_name);
                        z3_hard_constraint_expression =
                                _manager->substituteZ3Expression(z3_hard_constraint_expression, uninterpreted_constant,
                                                                 z3_reversioned_uninterpreted_constant);
                    } else if (uninterpreted_constant.is_int()) {
                        z3::expr z3_reversioned_uninterpreted_constant =
                                _manager->makeIntegerConstant(reversioned_name);
                        z3_hard_constraint_expression =
                                _manager->substituteZ3Expression(z3_hard_constraint_expression, uninterpreted_constant,
                                                                 z3_reversioned_uninterpreted_constant);
                    } else {
                        throw std::runtime_error("Unsupported z3::expr sort.");
                    }
                }
                std::string reversioned_name = contextualized_names_to_reversioned_names.at(hard_constraint_pair.first);
                hard_constraint_expressions.emplace(reversioned_name, z3_hard_constraint_expression);
            }
            hard_constraints.emplace(extractUncycledName(assumption_literal_name),
                                     std::move(hard_constraint_expressions));
        }
    }

    assert(!assumption_literals.empty());
    std::shared_ptr<AssumptionLiteralExpression> entry_assumption_literal =
            std::make_shared<AssumptionLiteralExpression>(
                    _manager->makeBooleanConstant(assumption_literals.begin()->first));
    std::shared_ptr<AssumptionLiteralExpression> exit_assumption_literal =
            std::make_shared<AssumptionLiteralExpression>(
                    _manager->makeBooleanConstant(std::prev(assumption_literals.end())->first));

    return std::make_unique<Summary>(std::move(entry_assumption_literal), std::move(exit_assumption_literal),
                                     std::move(assumption_literals), std::move(assumptions),
                                     std::move(hard_constraints));
}

std::string Summarizer::extractFlattenedName(const std::string &contextualized_name) {
    std::size_t version_position = contextualized_name.find('_');
    assert(version_position != std::string::npos);
    return contextualized_name.substr(0, version_position);
}

std::string Summarizer::extractUncycledName(const std::string &name) {
    std::size_t cycle_position = name.find("__");
    assert(cycle_position != std::string::npos);
    return name.substr(0, cycle_position);
}