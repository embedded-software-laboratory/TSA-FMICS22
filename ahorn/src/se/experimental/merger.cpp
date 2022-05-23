#include "se/experimental/merger.h"
#include "ir/instruction/call_instruction.h"
#include "se/experimental/expression/symbolic_expression.h"

#include "utilities/ostream_operators.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se;

Merger::Merger(const Configuration &configuration, Manager &manager)
    : _configuration(configuration), _manager(&manager),
      _merge_point_to_contexts(
              std::map<std::tuple<std::string, unsigned int, unsigned int>, std::vector<std::unique_ptr<Context>>>()) {}

std::ostream &Merger::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tmerge strategy: ";
    switch (_configuration.getMergeStrategy()) {
        case Configuration::MergeStrategy::AT_ALL_JOIN_POINTS: {
            str << "all join points";
            break;
        }
        case Configuration::MergeStrategy::ONLY_AT_CYCLE_END: {
            str << "only at cycle ends";
            break;
        }
        default:
            throw std::runtime_error("Invalid merge strategy configured.");
    }
    str << ",\n";
    str << "\tmerge points: ";
    str << "[";
    for (auto it = _merge_points.begin(); it != _merge_points.end(); ++it) {
        std::string scope = std::get<0>(*it);
        unsigned int label = std::get<1>(*it);
        unsigned int return_label = std::get<2>(*it);
        str << "(" << scope << ", " << label << ", " << return_label << ")";
        if (std::next(it) != _merge_points.end()) {
            str << ", ";
        }
    }
    str << "],\n";
    str << "\tmerge points to context queues: ";
    str << "{\n";
    for (auto it = _merge_point_to_contexts.begin(); it != _merge_point_to_contexts.end(); ++it) {
        std::string scope = std::get<0>(it->first);
        unsigned int label = std::get<1>(it->first);
        unsigned int return_label = std::get<2>(it->first);
        str << "\t\t";
        str << "(" << scope << ", " << label << ", " << return_label << ")";
        str << ": [";
        for (auto context = it->second.begin(); context != it->second.end(); ++context) {
            str << (*context)->getState().getVertex().getLabel();
            if (std::next(context) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]";
        if (std::next(it) != _merge_point_to_contexts.end()) {
            str << ",\n";
        }
    }
    str << "\n\t}\n";
    str << ")";
    return os << str.str();
}

bool Merger::isEmpty() const {
    bool is_empty = true;
    for (const auto &merge_point_to_contexts : _merge_point_to_contexts) {
        if (!merge_point_to_contexts.second.empty()) {
            is_empty = false;
            break;
        }
    }
    return is_empty;
}

void Merger::pushContext(std::unique_ptr<Context> context) {
    const Frame &frame = context->getFrame();
    unsigned int return_label = frame.getLabel();
    std::string scope = frame.getScope();
    const State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    unsigned int depth = context->getFrameDepth();
    assert(_merge_points.count(std::tuple(scope, label, return_label)) != 0);
    assert(_merge_point_to_contexts.count(std::tuple(scope, label, return_label)) != 0);
    _merge_point_to_contexts.at(std::tuple(scope, label, return_label)).push_back(std::move(context));
}

bool Merger::reachedMergePoint(const Context &context) const {
    const Frame &frame = context.getFrame();
    unsigned int return_label = frame.getLabel();
    std::string scope = frame.getScope();
    const State &state = context.getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    unsigned int depth = context.getFrameDepth();
    return _merge_points.count(std::tuple(scope, label, return_label)) != 0;
}

// TODO (04.01.2022): muss für depth-first und breadth-first heuristics und at all join points / at cycle end
//  angepasst werden.
std::unique_ptr<Context> Merger::merge() {
    auto logger = spdlog::get("Symbolic Execution");
    // TODO (27.01.2022): Enhance exploration to merge as much as possible, i.e., check if there any open contexts at
    //  a lesser depth which can potentially reach the most deep merge point and prioritize them.
    // XXX find the first non-empty context queue
    auto it = std::find_if(_merge_point_to_contexts.begin(), _merge_point_to_contexts.end(),
                           [](const auto &merge_point_to_context) { return !merge_point_to_context.second.empty(); });
    if (it != _merge_point_to_contexts.end()) {
        std::string scope = std::get<0>(it->first);
        unsigned int label = std::get<1>(it->first);
        SPDLOG_LOGGER_TRACE(logger, "Trying to merge contexts at merge point: ({}, {})", scope, label);
        std::unique_ptr<Context> merged_context = std::move(it->second.at(0));
        it->second.erase(it->second.begin());
        while (!it->second.empty()) {
            std::unique_ptr<Context> context = std::move(it->second.at(0));
            it->second.erase(it->second.begin());
            merged_context = merge(std::move(merged_context), std::move(context));
        }
        return merged_context;
    } else {
        throw std::runtime_error("No context for merging exists.");
    }
}

std::unique_ptr<Context> Merger::merge(std::unique_ptr<Context> context_1, std::unique_ptr<Context> context_2) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Merging {} with {}", *context_1, *context_2);

    unsigned int cycle_1 = context_1->getCycle();
    unsigned int cycle_2 = context_2->getCycle();
    assert(cycle_1 == cycle_2);

    unsigned int frame_depth_1 = context_1->getFrameDepth();
    unsigned int frame_depth_2 = context_2->getFrameDepth();
    assert(frame_depth_1 == frame_depth_2);

    const State &state_1 = context_1->getState();
    const Vertex &vertex_1 = state_1.getVertex();
    unsigned int label_1 = vertex_1.getLabel();
    const State &state_2 = context_2->getState();
    const Vertex &vertex_2 = state_2.getVertex();
    unsigned int label_2 = vertex_2.getLabel();
    assert(label_1 == label_2);

    const Frame &frame_1 = context_1->getFrame();
    std::string scope_1 = frame_1.getScope();
    const Frame &frame_2 = context_2->getFrame();
    std::string scope_2 = frame_2.getScope();
    assert(scope_1 == scope_2);

    const Cfg &cfg_1 = frame_1.getCfg();
    const Cfg &cfg_2 = frame_2.getCfg();
    assert(cfg_1.getType() == cfg_2.getType());

    const Frame &main_frame_1 = context_1->getMainFrame();
    std::string main_scope_1 = main_frame_1.getScope();
    const Frame &main_frame_2 = context_2->getMainFrame();
    std::string main_scope_2 = main_frame_2.getScope();
    assert(main_scope_1 == main_scope_2);

    const Cfg &main_cfg_1 = main_frame_1.getCfg();
    const Cfg &main_cfg_2 = main_frame_2.getCfg();
    assert(main_cfg_1.getType() == main_cfg_2.getType());

    // TODO (31.01.2022): Check if both contexts are called from within the same frame from the same return label ->
    //  only merge realizable paths.
    const std::deque<std::shared_ptr<Frame>> &frame_stack_1 = context_1->getFrameStack();
    const std::deque<std::shared_ptr<Frame>> &frame_stack_2 = context_2->getFrameStack();
    for (unsigned long frame_index = 0; frame_index != frame_stack_1.size(); ++frame_index) {
        assert(frame_stack_1.at(frame_index)->getScope() == frame_stack_2.at(frame_index)->getScope());
        assert(frame_stack_1.at(frame_index)->getLabel() == frame_stack_2.at(frame_index)->getLabel());
    }

    // underlying cfg, vertex and cycle for the merged context can be derived from either context, as merging always
    // occurs at a common merge point. We always use the main_cfg to access the flattened interface, as merging in
    // function blocks should merge on all valuations in the concrete and symbolic store.
    const Cfg &main_cfg = main_cfg_1;
    const Cfg &cfg = cfg_1;
    const Frame &frame = frame_1;
    const Vertex &vertex = vertex_1;
    unsigned int label = label_1;
    unsigned int cycle = cycle_1;

    // TODO (07.01.2022): The invariant, that the symbolic and concrete store, always have the same versions
    //  for each variable, respectively, should always hold. If we can assume this, it would make the code a
    //  lot easier.

    std::unique_ptr<Context> context;
    switch (_configuration.getEncodingMode()) {
        case Configuration::EncodingMode::NONE: {
            switch (_configuration.getMergeStrategy()) {
                case Configuration::MergeStrategy::AT_ALL_JOIN_POINTS: {
                    // merge path constraint
                    std::vector<std::shared_ptr<Expression>> path_constraint;
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_1 = state_1.getPathConstraint();
                    z3::expr_vector z3_path_constraint_1(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_1) {
                        z3_path_constraint_1.push_back(constraint->getZ3Expression().simplify());
                    }
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_2 = state_2.getPathConstraint();
                    z3::expr_vector z3_path_constraint_2(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_2) {
                        z3_path_constraint_2.push_back(constraint->getZ3Expression().simplify());
                    }
                    z3::expr z3_path_constraint =
                            (z3::mk_and(z3_path_constraint_1).simplify() || z3::mk_and(z3_path_constraint_2).simplify())
                                    .simplify();
                    path_constraint.push_back(std::make_shared<SymbolicExpression>(z3_path_constraint));

                    // merge concrete and symbolic store
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_concrete_expression;
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_symbolic_expression;
                    // XXX When in a call, this should merge on all variables, not only the flattened interface of the
                    // current cfg. We therefore need to access the "main" cfg which has all the flattened interface exposed.
                    // TODO (26.01.2022): This partially breaks the functionality, as it also adds hard constraints from
                    //  another scope to the current scope (e.g., from main program to callee fb)
                    for (auto it = main_cfg.flattenedInterfaceBegin(); it != main_cfg.flattenedInterfaceEnd(); ++it) {
                        std::string flattened_name = it->getFullyQualifiedName();
                        // TODO (28.01.2022): Improve retrieving maximum/minimum versions, idea: have fields for it
                        //  in the state and update when pushing new symbolic/concrete valuations?
                        unsigned int maximum_version_1 =
                                state_1.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_1 =
                                state_1.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int maximum_version_2 =
                                state_2.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_2 =
                                state_2.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        if (_manager->isWholeProgramInput(flattened_name)) {
                            if (maximum_version_1 == maximum_version_2) {
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Skipping the merge of {} as it is a \"whole-program\" input.",
                                                    flattened_name);
                            } else {
                                // TODO (08.01.2022): Consider assignments to the "whole-program" input variables, then we have
                                //  to merge.
                                throw std::logic_error("Not implemented yet.");
                            }
                        } else {
                            std::string contextualized_name_1 = flattened_name + "_" +
                                                                std::to_string(maximum_version_1) + "__" +
                                                                std::to_string(cycle);
                            std::string contextualized_name_2 = flattened_name + "_" +
                                                                std::to_string(maximum_version_2) + "__" +
                                                                std::to_string(cycle);
                            SPDLOG_LOGGER_TRACE(logger, "Merging on variable {} with versions {} and {}.",
                                                flattened_name, contextualized_name_1, contextualized_name_2);

                            // create a fresh new variable as lhs of the merge, depending on where and when the merge occurs,
                            // the maximum version + 1 of the two contexts might not be the highest version, hence retrieve
                            // the maximum version from the global version store of the manager and increment it
                            unsigned int version = _manager->getVersion(flattened_name) + 1;
                            _manager->setVersion(flattened_name, version);
                            std::string contextualized_name =
                                    flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                            const ir::DataType &data_type = it->getDataType();
                            z3::expr z3_variable = _manager->makeConstant(contextualized_name, data_type);

                            // TODO (10.01.2022): Why do we merge on their valuation? Why don't we just take the respective
                            //  symbolic variables? We already have their contextualized names...
                            std::shared_ptr<Expression> expression_1 =
                                    state_1.getSymbolicExpression(contextualized_name_1);
                            const z3::expr &z3_expression_1 = expression_1->getZ3Expression();
                            std::shared_ptr<Expression> expression_2 =
                                    state_2.getSymbolicExpression(contextualized_name_2);
                            const z3::expr &z3_expression_2 = expression_2->getZ3Expression();

                            bool equality = z3::eq(z3_expression_1, z3_expression_2);
                            if (equality) {
                                // TODO (08.01.2022): It probably can happen that two expressions have different versions but
                                //  are structurally equivalent -- what then?
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Expression {} and expression {} are structurally equivalent.",
                                                    *expression_1, *expression_2);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name, expression_1);
                                // update the concrete store for the merged variable, defaults to state_1
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            } else {
                                SPDLOG_LOGGER_TRACE(logger, "{} = {} and {} = {}", contextualized_name,
                                                    contextualized_name_1, contextualized_name, contextualized_name_2);

                                // (ii) of dimitri's encoding, update modified variable instances
                                z3::expr z3_variable_1 = _manager->makeConstant(contextualized_name_1, data_type);
                                z3::expr z3_variable_2 = _manager->makeConstant(contextualized_name_2, data_type);

                                // update the symbolic store
                                z3::expr z3_expression = z3::ite(z3::mk_and(z3_path_constraint_1).simplify(),
                                                                 z3_variable_1, z3_variable_2)
                                                                 .simplify();
                                std::shared_ptr<SymbolicExpression> symbolic_expression =
                                        std::make_shared<SymbolicExpression>(z3_expression);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name,
                                                                                   std::move(symbolic_expression));


                                // update the concrete store for the merged variable, defaults to state_1
                                // TODO (03.02.2022): What about example TestLibSe
                                //  .Engine_Call_Coverage_In_1_Cycle_With_Change_Shadow_Cfg? How to merge in case we
                                //  have a shadow expression?
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            }
                        }
                    }
                    /*
                    std::chrono::time_point<std::chrono::system_clock> begin_time_point =
                            std::chrono::system_clock::now();
                            */
                    // add prior versions of both states to the merged state, respectively, skip the merge value
                    for (auto valuation = state_1.concreteStoreBegin(); valuation != state_1.concreteStoreEnd();
                         ++valuation) {
                        contextualized_name_to_concrete_expression.emplace(valuation->first, valuation->second);
                    }
                    for (auto valuation = state_2.concreteStoreBegin(); valuation != state_2.concreteStoreEnd();
                         ++valuation) {
                        contextualized_name_to_concrete_expression.emplace(valuation->first, valuation->second);
                    }
                    for (auto valuation = state_1.symbolicStoreBegin(); valuation != state_1.symbolicStoreEnd();
                         ++valuation) {
                        contextualized_name_to_symbolic_expression.emplace(valuation->first, valuation->second);
                    }
                    for (auto valuation = state_2.symbolicStoreBegin(); valuation != state_2.symbolicStoreEnd();
                         ++valuation) {
                        contextualized_name_to_symbolic_expression.emplace(valuation->first, valuation->second);
                    }
                    /*
                    using namespace std::literals;
                    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
                    std::cout << "Elapsed time (all else in merge): " << elapsed_time << "ms" << std::endl;
                     */

                    // TODO (25.01.2022): Merging of shadow stores -- how?
                    std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression;
                    if (state_1.getShadowStore().empty() && state_2.getShadowStore().empty()) {
                        // do nothing
                    } else if (!state_1.getShadowStore().empty() && state_2.getShadowStore().empty()) {
                        shadow_name_to_shadow_expression = state_1.getShadowStore();
                    } else if (state_1.getShadowStore().empty() && !state_2.getShadowStore().empty()) {
                        shadow_name_to_shadow_expression = state_2.getShadowStore();
                    } else {
                        assert(state_1.getShadowStore().empty() && state_2.getShadowStore().empty());
                        for (const auto &shadow_valuation : state_1.getShadowStore()) {
                            shadow_name_to_shadow_expression.emplace(shadow_valuation);
                        }
                        for (const auto &shadow_valuation : state_2.getShadowStore()) {
                            auto it = std::find_if(shadow_name_to_shadow_expression.begin(),
                                                   shadow_name_to_shadow_expression.end(),
                                                   [shadow_valuation](const auto &valuation) {
                                                       return shadow_valuation.first == valuation.first;
                                                   });
                            if (it != shadow_name_to_shadow_expression.end()) {
                                throw std::runtime_error("Shadow name to shadow expression already exists but it "
                                                         "should be unique.");
                            }
                            shadow_name_to_shadow_expression.emplace(shadow_valuation);
                        }
                    }

                    std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                             VerificationConditionComparator>
                            assumption_literals;
                    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>,
                             VerificationConditionComparator>
                            assumptions;
                    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>
                            hard_constraints;
                    std::map<std::string, std::string, VerificationConditionComparator>
                            unknown_over_approximating_summary_literals;

                    // set the assumption literal of the state
                    std::unique_ptr<State> state = std::make_unique<State>(
                            _configuration, vertex, std::move(contextualized_name_to_concrete_expression),
                            std::move(contextualized_name_to_symbolic_expression),
                            std::move(shadow_name_to_shadow_expression), std::move(path_constraint),
                            std::shared_ptr<AssumptionLiteralExpression>(nullptr), std::move(assumption_literals),
                            std::move(assumptions), std::move(hard_constraints),
                            std::move(unknown_over_approximating_summary_literals));

                    // TODO (30.01.2022): What about the local path constraints inside the function blocks?

                    // TODO (27.01.2022): This is not true for contexts reaching the merge point from different call places.
                    // both, context_1 and context_2 merge at the same label, hence their frames are equal and either can be
                    // taken
                    std::deque<std::shared_ptr<Frame>> frame_stack_1 = context_1->getFrameStack();
                    std::cout << "frame_stack_1: " << frame_stack_1 << std::endl;
                    std::deque<std::shared_ptr<Frame>> frame_stack_2 = context_2->getFrameStack();
                    std::cout << "frame_stack_2: " << frame_stack_2 << std::endl;

                    std::deque<std::shared_ptr<Frame>> frame_stack;
                    for (unsigned long frame_index = 0; frame_index != frame_stack_1.size(); ++frame_index) {
                        std::shared_ptr<Frame> frame_1 = frame_stack_1.at(frame_index);
                        std::shared_ptr<Frame> frame_2 = frame_stack_2.at(frame_index);
                        assert(frame_1->getScope() == frame_2->getScope());
                        assert(frame_1->getLabel() == frame_2->getLabel());
                        z3::expr_vector z3_local_path_constraint_1(_manager->getZ3Context());
                        for (const auto &local_constraint : frame_1->getLocalPathConstraint()) {
                            z3_local_path_constraint_1.push_back(local_constraint->getZ3Expression());
                        }
                        z3::expr_vector z3_local_path_constraint_2(_manager->getZ3Context());
                        for (const auto &local_constraint : frame_2->getLocalPathConstraint()) {
                            z3_local_path_constraint_2.push_back(local_constraint->getZ3Expression());
                        }
                        z3::expr z3_local_path_constraint =
                                (z3::mk_and(z3_local_path_constraint_1) || z3::mk_and(z3_local_path_constraint_2))
                                        .simplify();
                        std::shared_ptr<Frame> frame =
                                std::make_shared<Frame>(frame_1->getCfg(), frame_1->getScope(), frame_1->getLabel());
                        frame->pushLocalPathConstraint(std::make_shared<SymbolicExpression>(z3_local_path_constraint));
                        frame_stack.push_back(frame);
                    }
                    std::cout << "resulting frame stack: " << frame_stack << std::endl;

                    // TODO (27.01.2022): Can both contexts reach the same function block from different frame
                    //  stacks? Yes, see for example Engine_Antivalent_Test_Cfg, what now?
                    context = std::make_unique<Context>(cycle, std::move(state), std::move(frame_stack));
                    break;
                }
                case Configuration::MergeStrategy::ONLY_AT_CYCLE_END: {
                    // merge path constraint
                    std::vector<std::shared_ptr<Expression>> path_constraint;
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_1 = state_1.getPathConstraint();
                    z3::expr_vector z3_path_constraint_1(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_1) {
                        z3_path_constraint_1.push_back(constraint->getZ3Expression().simplify());
                    }
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_2 = state_2.getPathConstraint();
                    z3::expr_vector z3_path_constraint_2(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_2) {
                        z3_path_constraint_2.push_back(constraint->getZ3Expression().simplify());
                    }
                    z3::expr z3_path_constraint =
                            (z3::mk_and(z3_path_constraint_1).simplify() || z3::mk_and(z3_path_constraint_2).simplify())
                                    .simplify();
                    path_constraint.push_back(std::make_shared<SymbolicExpression>(z3_path_constraint));

                    // merge concrete and symbolic store
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_concrete_expression;
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_symbolic_expression;
                    for (auto it = main_cfg.flattenedInterfaceBegin(); it != main_cfg.flattenedInterfaceEnd(); ++it) {
                        std::string flattened_name = it->getFullyQualifiedName();
                        unsigned int maximum_version_1 =
                                state_1.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_1 =
                                state_1.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int maximum_version_2 =
                                state_2.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_2 =
                                state_2.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        if (_manager->isWholeProgramInput(flattened_name)) {
                            if (maximum_version_1 == maximum_version_2) {
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Skipping the merge of {} as it is a \"whole-program\" input.",
                                                    flattened_name);
                            } else {
                                // TODO (08.01.2022): Consider assignments to the "whole-program" input variables, then we have
                                //  to merge.
                                throw std::logic_error("Not implemented yet.");
                            }
                        } else {
                            std::string contextualized_name_1 = flattened_name + "_" +
                                                                std::to_string(maximum_version_1) + "__" +
                                                                std::to_string(cycle);
                            std::string contextualized_name_2 = flattened_name + "_" +
                                                                std::to_string(maximum_version_2) + "__" +
                                                                std::to_string(cycle);
                            SPDLOG_LOGGER_TRACE(logger, "Merging on variable {} with versions {} and {}.",
                                                flattened_name, contextualized_name_1, contextualized_name_2);

                            // create a fresh new variable as lhs of the merge, depending on where and when the merge occurs,
                            // the maximum version + 1 of the two contexts might not be the highest version, hence retrieve
                            // the maximum version from the global version store of the manager and increment it
                            unsigned int version = _manager->getVersion(flattened_name) + 1;
                            _manager->setVersion(flattened_name, version);
                            std::string contextualized_name =
                                    flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                            const ir::DataType &data_type = it->getDataType();
                            z3::expr z3_variable = _manager->makeConstant(contextualized_name, data_type);

                            // TODO (10.01.2022): Why do we merge on their valuation? Why don't we just take the respective
                            //  symbolic variables? We already have their contextualized names...
                            std::shared_ptr<Expression> expression_1 =
                                    state_1.getSymbolicExpression(contextualized_name_1);
                            const z3::expr &z3_expression_1 = expression_1->getZ3Expression();
                            std::shared_ptr<Expression> expression_2 =
                                    state_2.getSymbolicExpression(contextualized_name_2);
                            const z3::expr &z3_expression_2 = expression_2->getZ3Expression();
                            if (z3::eq(z3_expression_1, z3_expression_2)) {
                                // TODO (08.01.2022): It probably can happen that two expressions have different versions but
                                //  are structurally equivalent -- what then?
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Expression {} and expression {} are structurally equivalent.",
                                                    *expression_1, *expression_2);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name, expression_1);
                                // update the concrete store for the merged variable, defaults to state_1
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            } else {
                                SPDLOG_LOGGER_TRACE(logger, "{} = {} and {} = {}", contextualized_name,
                                                    contextualized_name_1, contextualized_name, contextualized_name_2);

                                // (ii) of dimitri's encoding, update modified variable instances
                                z3::expr z3_variable_1 = _manager->makeConstant(contextualized_name_1, data_type);
                                z3::expr z3_variable_2 = _manager->makeConstant(contextualized_name_2, data_type);

                                // update the symbolic store
                                z3::expr z3_expression = z3::ite(z3::mk_and(z3_path_constraint_1).simplify(),
                                                                 z3_variable_1, z3_variable_2)
                                                                 .simplify();
                                std::shared_ptr<SymbolicExpression> symbolic_expression =
                                        std::make_shared<SymbolicExpression>(z3_expression);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name,
                                                                                   std::move(symbolic_expression));


                                // update the concrete store for the merged variable, defaults to state_1
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            }
                        }
                    }
                    // add prior versions of both states to the merged state, respectively, skip the merge value
                    for (const auto &concrete_valuation : state_1.getConcreteStore()) {
                        contextualized_name_to_concrete_expression.emplace(concrete_valuation.first,
                                                                           concrete_valuation.second);
                    }
                    for (const auto &concrete_valuation : state_2.getConcreteStore()) {
                        contextualized_name_to_concrete_expression.emplace(concrete_valuation.first,
                                                                           concrete_valuation.second);
                    }
                    for (const auto &symbolic_valuation : state_1.getSymbolicStore()) {
                        contextualized_name_to_symbolic_expression.emplace(symbolic_valuation.first,
                                                                           symbolic_valuation.second);
                    }
                    for (const auto &symbolic_valuation : state_2.getSymbolicStore()) {
                        contextualized_name_to_symbolic_expression.emplace(symbolic_valuation.first,
                                                                           symbolic_valuation.second);
                    }
                    // TODO (25.01.2022): Merging of shadow stores -- how?
                    std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression;
                    if (!state_1.getShadowStore().empty() || !state_2.getShadowStore().empty()) {
                        throw std::logic_error("Not implemented yet.");
                    }

                    std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                             VerificationConditionComparator>
                            assumption_literals;
                    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>,
                             VerificationConditionComparator>
                            assumptions;
                    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>
                            hard_constraints;
                    std::map<std::string, std::string, VerificationConditionComparator>
                            unknown_over_approximating_summary_literals;

                    // set the assumption literal of the state
                    std::unique_ptr<State> state = std::make_unique<State>(
                            _configuration, vertex, std::move(contextualized_name_to_concrete_expression),
                            std::move(contextualized_name_to_symbolic_expression),
                            std::move(shadow_name_to_shadow_expression), std::move(path_constraint),
                            std::shared_ptr<AssumptionLiteralExpression>(nullptr), std::move(assumption_literals),
                            std::move(assumptions), std::move(hard_constraints),
                            std::move(unknown_over_approximating_summary_literals));

                    // TODO (27.01.2022): This is not true for contexts reaching the merge point from different call places.
                    // both, context_1 and context_2 merge at the same label, hence their frames are equal and either can be
                    // taken
                    std::deque<std::shared_ptr<Frame>> frame_stack = context_1->getFrameStack();
                    std::deque<std::shared_ptr<Frame>> frame_stack_2 = context_2->getFrameStack();
                    // TODO (27.01.2022): Can both contexts reach the same function block from different frame stacks?
                    context = std::make_unique<Context>(cycle, std::move(state), std::move(frame_stack));
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected merge strategy encountered.");
            }
            break;
        }
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal_1 = state_1.getAssumptionLiteral();
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal_2 = state_2.getAssumptionLiteral();
            assert(z3::eq(assumption_literal_1->getZ3Expression(), assumption_literal_2->getZ3Expression()));
            std::shared_ptr<AssumptionLiteralExpression> assumption_literal = assumption_literal_1;
            std::string assumption_literal_name = assumption_literal->getZ3Expression().to_string();
            assert(assumption_literal_name ==
                   "b_" + frame.getScope() + "_" + std::to_string(label) + "__" + std::to_string(cycle));
            switch (_configuration.getMergeStrategy()) {
                case Configuration::MergeStrategy::AT_ALL_JOIN_POINTS: {
                    // merge path constraint
                    std::vector<std::shared_ptr<Expression>> path_constraint;
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_1 = state_1.getPathConstraint();
                    z3::expr_vector z3_path_constraint_1(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_1) {
                        z3_path_constraint_1.push_back(constraint->getZ3Expression());
                    }
                    const std::vector<std::shared_ptr<Expression>> &path_constraint_2 = state_2.getPathConstraint();
                    z3::expr_vector z3_path_constraint_2(_manager->getZ3Context());
                    for (const std::shared_ptr<Expression> &constraint : path_constraint_2) {
                        z3_path_constraint_2.push_back(constraint->getZ3Expression());
                    }
                    z3::expr z3_path_constraint =
                            (z3::mk_and(z3_path_constraint_1) || z3::mk_and(z3_path_constraint_2)).simplify();
                    path_constraint.push_back(std::make_shared<SymbolicExpression>(z3_path_constraint));

                    // preceding assumption literals that join control-flow at this merge point represented by the assumption
                    // literal
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>> assumption_literals_1 =
                            state_1.getAssumptionLiterals(assumption_literal_name);
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>> assumption_literals_2 =
                            state_2.getAssumptionLiterals(assumption_literal_name);

                    // merge concrete and symbolic store
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_concrete_expression;
                    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                            contextualized_name_to_symbolic_expression;

                    // maps the contextualized name of the modified variable to the merged variable expression
                    std::map<std::string, std::string> modified_variable_instances;

                    // XXX When in a call, this should merge on all variables, not only the flattened interface of the
                    // current cfg. We therefore need to access the "main" cfg which has all the flattened interface exposed.
                    // TODO (26.01.2022): This partially breaks the functionality, as it also adds hard constraints from
                    //  another scope to the current scope (e.g., from main program to callee fb)
                    for (auto it = main_cfg.flattenedInterfaceBegin(); it != main_cfg.flattenedInterfaceEnd(); ++it) {
                        std::string flattened_name = it->getFullyQualifiedName();
                        unsigned int maximum_version_1 =
                                state_1.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_1 =
                                state_1.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int maximum_version_2 =
                                state_2.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
                        unsigned int minimum_version_2 =
                                state_2.getMinimumVersionFromSymbolicStore(flattened_name, cycle);
                        if (_manager->isWholeProgramInput(flattened_name)) {
                            if (maximum_version_1 == maximum_version_2) {
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Skipping the merge of {} as it is a \"whole-program\" input.",
                                                    flattened_name);
                            } else {
                                // TODO (08.01.2022): Consider assignments to the "whole-program" input variables, then we have
                                //  to merge.
                                throw std::logic_error("Not implemented yet.");
                            }
                        } else {
                            std::string contextualized_name_1 = flattened_name + "_" +
                                                                std::to_string(maximum_version_1) + "__" +
                                                                std::to_string(cycle);
                            std::string contextualized_name_2 = flattened_name + "_" +
                                                                std::to_string(maximum_version_2) + "__" +
                                                                std::to_string(cycle);
                            SPDLOG_LOGGER_TRACE(logger, "Merging on variable {} with versions {} and {}.",
                                                flattened_name, contextualized_name_1, contextualized_name_2);

                            // create a fresh new variable as lhs of the merge, depending on where and when the merge occurs,
                            // the maximum version + 1 of the two contexts might not be the highest version, hence retrieve
                            // the maximum version from the global version store of the manager and increment it
                            unsigned int version = _manager->getVersion(flattened_name) + 1;
                            _manager->setVersion(flattened_name, version);
                            std::string contextualized_name =
                                    flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
                            const ir::DataType &data_type = it->getDataType();
                            z3::expr z3_variable = _manager->makeConstant(contextualized_name, data_type);

                            // TODO (10.01.2022): Why do we merge on their valuation? Why don't we just take the respective
                            //  symbolic variables? We already have their contextualized names...
                            std::shared_ptr<Expression> expression_1 =
                                    state_1.getSymbolicExpression(contextualized_name_1);
                            const z3::expr &z3_expression_1 = expression_1->getZ3Expression();
                            std::shared_ptr<Expression> expression_2 =
                                    state_2.getSymbolicExpression(contextualized_name_2);
                            const z3::expr &z3_expression_2 = expression_2->getZ3Expression();
                            if (z3::eq(z3_expression_1, z3_expression_2)) {
                                // TODO (08.01.2022): It probably can happen that two expressions have different versions but
                                //  are structurally equivalent -- what then?
                                SPDLOG_LOGGER_TRACE(logger,
                                                    "Expression {} and expression {} are structurally equivalent.",
                                                    *expression_1, *expression_2);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name, expression_1);
                                // update the concrete store for the merged variable, defaults to state_1
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            } else {
                                SPDLOG_LOGGER_TRACE(logger, "{} -> {} = {} and {} -> {} = {}", assumption_literals_1,
                                                    contextualized_name, contextualized_name_1, assumption_literals_2,
                                                    contextualized_name, contextualized_name_2);

                                // (ii) of dimitri's encoding, update modified variable instances
                                z3::expr z3_variable_1 = _manager->makeConstant(contextualized_name_1, data_type);
                                z3::expr z3_variable_2 = _manager->makeConstant(contextualized_name_2, data_type);

                                modified_variable_instances.emplace(contextualized_name_1, contextualized_name);
                                modified_variable_instances.emplace(contextualized_name_2, contextualized_name);

                                // TODO (29.01.2022): Change modified variable instances to a map to prevent adding
                                //  multiple entries, e.g.,
                                //  b_P.f_2__0: [
                                //(P.f.b_6__0 = P.f.b_4__0)
                                //(P.f.b_6__0 = P.f.b_5__0)
                                //]
                                //b_P.f_4__0: [
                                //(P.f.b_6__0 = P.f.b_4__0)
                                //(P.f.b_6__0 = P.f.b_5__0)
                                //]
                                // die story ist: wir sind an einem join punkt und beide branches haben die variable
                                // "b" verändert. wir können aber jetzt nicht in beide die modifications pushen,
                                // diese müssen eindeutig sein und vor allem dem control-flow entsprechen
                                // wir müssen wissen woher wir kamen ->
                                // TODO (30.01.2022): Change hard constraints to a map where we can identify the key
                                //  to be the lhs of the assignment and hence the written variable. This would
                                //  simplify the merging. -> this won't work that easily, because we also have the
                                //  call dependency
                                // Solution:
                                // 1. Bestimme welche Variablen sich verändert haben, e.g., P.f.b_4__0 und P.f.b_5__0
                                // 2. Gehe durch die hard constraints an den assumption literalen die den join point
                                // erreichen in beiden Kontexten durch und prüfe ob die veränderten variablen dort
                                // geschrieben wurden oder nicht.
                                // 3. Füge jedem hard constraints wo sich die veränderte variable befindet das neue
                                // hard constraint hinzu welches die veränderte variable auf die neu
                                // erstellte gemeinsame variable setzt.

                                // update the symbolic store
                                z3::expr z3_expression = z3::ite(z3::mk_and(z3_path_constraint_1).simplify(),
                                                                 z3_variable_1, z3_variable_2)
                                                                 .simplify();
                                std::shared_ptr<SymbolicExpression> symbolic_expression =
                                        std::make_shared<SymbolicExpression>(z3_expression);
                                contextualized_name_to_symbolic_expression.emplace(contextualized_name,
                                                                                   std::move(symbolic_expression));


                                // update the concrete store for the merged variable, defaults to state_1
                                std::shared_ptr<Expression> concrete_expression =
                                        state_1.getConcreteExpression(contextualized_name_1);
                                contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                                                   std::move(concrete_expression));
                            }
                        }
                    }
                    // add prior versions of both states to the merged state, respectively, skip the merge value
                    for (const auto &concrete_valuation : state_1.getConcreteStore()) {
                        contextualized_name_to_concrete_expression.emplace(concrete_valuation.first,
                                                                           concrete_valuation.second);
                    }
                    for (const auto &concrete_valuation : state_2.getConcreteStore()) {
                        contextualized_name_to_concrete_expression.emplace(concrete_valuation.first,
                                                                           concrete_valuation.second);
                    }
                    for (const auto &symbolic_valuation : state_1.getSymbolicStore()) {
                        contextualized_name_to_symbolic_expression.emplace(symbolic_valuation.first,
                                                                           symbolic_valuation.second);
                    }
                    for (const auto &symbolic_valuation : state_2.getSymbolicStore()) {
                        contextualized_name_to_symbolic_expression.emplace(symbolic_valuation.first,
                                                                           symbolic_valuation.second);
                    }

                    // TODO (05.01.2022): Siehe Dimitris Dissertation S. 72 -> Three steps merging
                    // merging verification conditions
                    SPDLOG_LOGGER_TRACE(logger, "Merging assumption literals from {} and {}...", assumption_literals_1,
                                        assumption_literals_2);
                    std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                             VerificationConditionComparator>
                            assumption_literals;
                    for (const auto &ass_lits_1 : state_1.getAssumptionLiterals()) {
                        if (assumption_literals.count(ass_lits_1.first) ||
                            ass_lits_1.first == assumption_literal_name) {
                            continue;
                        } else {
                            std::vector<std::shared_ptr<AssumptionLiteralExpression>> ass_lits;
                            for (const auto &ass_lit : ass_lits_1.second) {
                                ass_lits.push_back(ass_lit);
                            }
                            assumption_literals.emplace(ass_lits_1.first, std::move(ass_lits));
                        }
                    }
                    for (const auto &ass_lits_2 : state_2.getAssumptionLiterals()) {
                        if (assumption_literals.count(ass_lits_2.first) ||
                            ass_lits_2.first == assumption_literal_name) {
                            continue;
                        } else {
                            std::vector<std::shared_ptr<AssumptionLiteralExpression>> ass_lits;
                            for (const auto &ass_lit : ass_lits_2.second) {
                                ass_lits.push_back(ass_lit);
                            }
                            assumption_literals.emplace(ass_lits_2.first, std::move(ass_lits));
                        }
                    }

                    // (i) of dimitri's encoding
                    // add all prior assumption literals from both states, if an assumption literal already exists, skip
                    // it, because this means both paths have the same prefix -> merge assumption literals, do not add
                    // duplicates
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>> ass_lits;
                    ass_lits.reserve(assumption_literals_1.size());
                    for (const auto &ass_lit_1 : assumption_literals_1) {
                        ass_lits.push_back(ass_lit_1);
                    }
                    for (const auto &ass_lit_2 : assumption_literals_2) {
                        auto it = std::find_if(ass_lits.begin(), ass_lits.end(), [ass_lit_2](const auto &ass_lit) {
                            return z3::eq(ass_lit->getZ3Expression(), ass_lit_2->getZ3Expression());
                        });
                        if (it == ass_lits.end()) {
                            ass_lits.push_back(ass_lit_2);
                        }
                    }
                    SPDLOG_LOGGER_TRACE(logger, "{}", ass_lits);
                    assumption_literals.emplace(assumption_literal_name, std::move(ass_lits));

                    std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>,
                             VerificationConditionComparator>
                            assumptions;
                    SPDLOG_LOGGER_TRACE(logger, "Merging assumptions...");
                    for (const auto &assumptions_1 : state_1.getAssumptions()) {
                        if (assumptions.count(assumptions_1.first)) {
                            continue;
                        } else {
                            std::vector<std::shared_ptr<SymbolicExpression>> expressions;
                            for (const auto &expression : assumptions_1.second) {
                                expressions.push_back(expression);
                            }
                            assumptions.emplace(assumptions_1.first, std::move(expressions));
                        }
                    }
                    for (const auto &assumptions_2 : state_2.getAssumptions()) {
                        if (assumptions.count(assumptions_2.first)) {
                            continue;
                        } else {
                            std::vector<std::shared_ptr<SymbolicExpression>> expressions;
                            for (const auto &expression : assumptions_2.second) {
                                expressions.push_back(expression);
                            }
                            assumptions.emplace(assumptions_2.first, std::move(expressions));
                        }
                    }

                    SPDLOG_LOGGER_TRACE(logger, "Merging hard constraints...");
                    std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>
                            hard_constraints;
                    for (const auto &hard_constraints_1 : state_1.getHardConstraints()) {
                        std::map<std::string, z3::expr> expressions;
                        for (const auto &expression : hard_constraints_1.second) {
                            expressions.emplace(expression.first, expression.second);
                        }
                        hard_constraints.emplace(hard_constraints_1.first, std::move(expressions));
                    }
                    for (const auto &hard_constraints_2 : state_2.getHardConstraints()) {
                        if (hard_constraints.count(hard_constraints_2.first) == 0) {
                            std::map<std::string, z3::expr> expressions;
                            for (const auto &expression : hard_constraints_2.second) {
                                expressions.emplace(expression.first, expression.second);
                            }
                            hard_constraints.emplace(hard_constraints_2.first, std::move(expressions));
                        } else {
                            for (const auto &expression : hard_constraints_2.second) {
                                hard_constraints.at(hard_constraints_2.first)
                                        .emplace(expression.first, expression.second);
                            }
                        }
                    }

                    // b_P.f_2__0:
                    // std::string -> z3::expr
                    // "P.f.b_4__0" -> P.f.b_6__0
                    // "P.f.b_5__0" -> P.f.b_6__0
                    //
                    // b_P.f_4__0:
                    // std::string -> z3::expr
                    // "P.f.b_4__0" -> P.f.b_6__0
                    // "P.f.b_5__0" -> P.f.b_6__0
                    // (iii) of dimitri's encoding
                    // 1) collect all preceding assumption literals
                    std::vector<std::shared_ptr<AssumptionLiteralExpression>> preceding_assumption_literals =
                            assumption_literals_1;
                    for (const auto &ass_lit_2 : assumption_literals_2) {
                        auto it =
                                std::find_if(preceding_assumption_literals.begin(), preceding_assumption_literals.end(),
                                             [&ass_lit_2](const auto &preceding_ass_lit) {
                                                 return ass_lit_2->getZ3Expression().to_string() ==
                                                        preceding_ass_lit->getZ3Expression().to_string();
                                             });
                        if (it == preceding_assumption_literals.end()) {
                            preceding_assumption_literals.push_back(ass_lit_2);
                        }
                    }
                    // 2) for each preceding assumption literal, check the hard constraints. If the hard constraints
                    // contain the respective variable, add a new valuation to the respective hard constraint
                    for (const auto &preceding_assumption_literal : preceding_assumption_literals) {
                        std::string preceding_assumption_literal_name =
                                preceding_assumption_literal->getZ3Expression().to_string();
                        // XXX not every preceding assumption literal has hard constraints, e.g., in if-chains
                        if (hard_constraints.count(preceding_assumption_literal_name)) {
                            for (const auto &p : hard_constraints.at(preceding_assumption_literal_name)) {
                                if (modified_variable_instances.count(p.first) != 0) {
                                    if (p.second.is_bool()) {
                                        hard_constraints.at(preceding_assumption_literal_name)
                                                .emplace(modified_variable_instances.at(p.first),
                                                         _manager->makeBooleanConstant(p.first));
                                    } else if (p.second.is_int()) {
                                        hard_constraints.at(preceding_assumption_literal_name)
                                                .emplace(modified_variable_instances.at(p.first),
                                                         _manager->makeIntegerConstant(p.first));
                                    } else {
                                        throw std::runtime_error("Unsupported z3::sort encountered.");
                                    }
                                }
                            }
                        } else {
                            SPDLOG_LOGGER_TRACE(logger,
                                                "Preceding assumption literal {} does not have any hard "
                                                "constraints.",
                                                preceding_assumption_literal_name);
                        }
                    }

                    SPDLOG_LOGGER_TRACE(logger, "Merging unknown over-approximating summary literals...");
                    std::map<std::string, std::string, VerificationConditionComparator>
                            unknown_over_approximating_summary_literals =
                                    state_1.getUnknownOverApproximatingSummaryLiterals();
                    for (const auto &p : state_2.getUnknownOverApproximatingSummaryLiterals()) {
                        if (unknown_over_approximating_summary_literals.count(p.first) == 0) {
                            unknown_over_approximating_summary_literals.emplace(p.first, p.second);
                        } else {
                            if (unknown_over_approximating_summary_literals.at(p.first) != p.second) {
                                // we expect the same summary, i.e., the same exit literal at the same locations.
                                throw std::logic_error("Not implemented yet.");
                            }
                        }
                    }

                    // TODO (18.01.2022): The way we have implemented the generation of the assumption literals ensures that
                    //  at the merge point the contexts to merge have the same assumption literal, i.e., the assumption
                    //  literal of that specific label / vertex. Either merging with an already merged state or with a fresh
                    //  state on this merge point.

                    // TODO (25.01.2022): Merging of shadow stores -- how?
                    std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression;
                    if (!state_1.getShadowStore().empty() || !state_2.getShadowStore().empty()) {
                        throw std::logic_error("Not implemented yet.");
                    }

                    // set the assumption literal of the state
                    std::unique_ptr<State> state = std::make_unique<State>(
                            _configuration, vertex, std::move(contextualized_name_to_concrete_expression),
                            std::move(contextualized_name_to_symbolic_expression),
                            std::move(shadow_name_to_shadow_expression), std::move(path_constraint), assumption_literal,
                            std::move(assumption_literals), std::move(assumptions), std::move(hard_constraints),
                            std::move(unknown_over_approximating_summary_literals));

                    // TODO (27.01.2022): Can both contexts reach the same function block from different frame stacks?
                    std::deque<std::shared_ptr<Frame>> frame_stack = mergeFrameStack(*context_1, *context_2);

                    context = std::make_unique<Context>(cycle, std::move(state), std::move(frame_stack));
                    break;
                }
                case Configuration::MergeStrategy::ONLY_AT_CYCLE_END: {
                    // XXX for now, merge only at the exit (or entry of a cycle, respectively)
                    assert(cfg_1.getType() == Cfg::Type::PROGRAM);
                    assert(cfg_1.getEntryLabel() == label_1);
                    assert(cfg_2.getEntryLabel() == label_2);
                    throw std::logic_error("Not implemented yet.");
                    break;
                }
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }

    assert(context != nullptr);
    SPDLOG_LOGGER_TRACE(logger, "Resulting merged context: {}", *context);
    return context;
}

// XXX Implementation is invalid for contexts reaching the merge point from different call sites.
std::deque<std::shared_ptr<Frame>> Merger::mergeFrameStack(const Context &context_1, const Context &context_2) {
    auto logger = spdlog::get("Symbolic Execution");

    std::deque<std::shared_ptr<Frame>> frame_stack;
    const std::deque<std::shared_ptr<Frame>> &frame_stack_1 = context_1.getFrameStack();
    const std::deque<std::shared_ptr<Frame>> &frame_stack_2 = context_2.getFrameStack();
    SPDLOG_LOGGER_TRACE(logger, "Merging frame stack {} with frame stack {}.", frame_stack_1, frame_stack_2);

    assert(frame_stack_1.size() == frame_stack_2.size());
    for (unsigned long index = 0; index != frame_stack_1.size(); ++index) {
        const Frame &frame_1 = *frame_stack_1.at(index);
        const Frame &frame_2 = *frame_stack_2.at(index);
        assert(frame_1.getScope() == frame_2.getScope());
        assert(frame_1.getLabel() == frame_2.getLabel());
        z3::expr_vector z3_local_path_constraint_1(_manager->getZ3Context());
        for (const auto &local_constraint : frame_1.getLocalPathConstraint()) {
            z3_local_path_constraint_1.push_back(local_constraint->getZ3Expression());
        }
        z3::expr_vector z3_local_path_constraint_2(_manager->getZ3Context());
        for (const auto &local_constraint : frame_2.getLocalPathConstraint()) {
            z3_local_path_constraint_2.push_back(local_constraint->getZ3Expression());
        }
        z3::expr z3_local_path_constraint =
                (z3::mk_and(z3_local_path_constraint_1) || z3::mk_and(z3_local_path_constraint_2)).simplify();
        std::shared_ptr<Frame> frame =
                std::make_shared<Frame>(frame_1.getCfg(), frame_1.getScope(), frame_1.getLabel());
        frame->pushLocalPathConstraint(std::make_shared<SymbolicExpression>(z3_local_path_constraint));
        frame_stack.push_back(frame);
    }

    SPDLOG_LOGGER_TRACE(logger, "Resulting merged frame stack: {}", frame_stack);
    return frame_stack;
}

void Merger::initialize(const Cfg &cfg) {
    // collect potential locations for merging
    unsigned int depth = 1;// depth starting at 1 due to all contexts running in main frame
    std::string scope = cfg.getName();
    std::set<std::string> visited_cfgs;
    initializeMergePoints(cfg, depth, scope, cfg.getEntryLabel(), visited_cfgs);
    // initialize context queues
    for (const auto &merge_point : _merge_points) {
        _merge_point_to_contexts.emplace(merge_point, std::vector<std::unique_ptr<Context>>());
    }
}

void Merger::initializeMergePoints(const Cfg &cfg, unsigned int depth, std::string scope, unsigned int return_label,
                                   std::set<std::string> &visited_cfgs) {
    std::string name = cfg.getName();
    // assert(visited_cfgs.find(name) == visited_cfgs.end());
    // visited_cfgs.insert(name);

    switch (_configuration.getMergeStrategy()) {
        case Configuration::MergeStrategy::AT_ALL_JOIN_POINTS: {
            // XXX recurse on callees
            /*for (auto it = cfg.calleesBegin(); it != cfg.calleesEnd(); ++it) {
                if (visited_cfgs.count(it->getName()) == 0) {
                    initializeMergePoints(*it, depth + 1, visited_cfgs);
                }
            }*/
            for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
                switch (it->getType()) {
                    case Vertex::Type::ENTRY: {
                        break;
                    }
                    case Vertex::Type::REGULAR: {
                        unsigned int label = it->getLabel();
                        std::vector<unsigned int> preceding_labels = cfg.getPrecedingLabels(it->getLabel());
                        if (preceding_labels.size() > 1) {
                            bool eligible_merge_point = true;
                            for (const auto &incoming_edge : cfg.getIncomingEdges(label)) {
                                if (incoming_edge->getType() != Edge::Type::INTRAPROCEDURAL) {
                                    eligible_merge_point = false;
                                }
                            }
                            if (eligible_merge_point) {
                                _merge_points.emplace(scope, label, return_label);
                            }
                        }
                        if (auto call_instruction = dynamic_cast<const ir::CallInstruction *>(it->getInstruction())) {
                            std::string callee_scope = scope + "." + call_instruction->getVariableAccess().getName();
                            std::shared_ptr<Cfg> callee = cfg.getCallee(label);
                            const auto &edge = cfg.getIntraproceduralCallToReturnEdge(label);
                            //if (visited_cfgs.count(callee->getName()) == 0) {
                            initializeMergePoints(*callee, depth, callee_scope, edge.getTargetLabel(), visited_cfgs);
                            //}
                        }
                        break;
                    }
                    case Vertex::Type::EXIT: {
                        _merge_points.emplace(scope, cfg.getExitLabel(), return_label);
                        /*
                        if (cfg.getType() == Cfg::Type::PROGRAM) {
                            _merge_points.emplace(scope, cfg.getExitLabel(), return_label);
                        } else {
                            std::cout << name << " gets called from: " << std::endl;
                            for (auto caller = cfg.callersBegin(); caller != cfg.callersEnd(); ++caller) {
                                std::cout << caller->getName() << std::endl;
                                std::vector<unsigned int> call_labels = caller->getCallLabels(name);
                                for (const auto call_label : call_labels) {
                                    const Vertex &call_vertex = caller->getVertex(call_label);
                                    auto call_instruction =
                                            dynamic_cast<const ir::CallInstruction *>(call_vertex.getInstruction());
                                    const auto &intraprocedural_call_to_return_edge =
                                            caller->getIntraproceduralCallToReturnEdge(call_label);
                                    unsigned int return_label = intraprocedural_call_to_return_edge.getTargetLabel();
                                    std::cout << call_label << " : " << call_instruction->getVariableAccess().getName()
                                              << " : " << return_label << std::endl;
                                    _merge_points.emplace(scope, cfg.getExitLabel(), return_label);
                                }
                            }
                            std::cout << "in scope: " << scope << std::endl;
                        }
                        */
                        break;
                    }
                }
            }
            break;
        }
        case Configuration::MergeStrategy::ONLY_AT_CYCLE_END: {
            // XXX only merge at program exit
            _merge_points.emplace(scope, cfg.getExitLabel(), return_label);
            break;
        }
    }
}
