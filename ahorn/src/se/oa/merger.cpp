#include "se/oa/merger.h"
#include "ir/instruction/call_instruction.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::oa;

Merger::Merger(Solver &solver, Executor &executor)
    : _solver(&solver), _executor(&executor), _merge_points(std::set<merge_point_t>()),
      _merge_point_to_contexts(std::map<merge_point_t, std::vector<std::unique_ptr<Context>>>()) {}

std::ostream &Merger::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tmerge points to context queues: ";
    str << "{\n";
    for (auto it = _merge_point_to_contexts.begin(); it != _merge_point_to_contexts.end(); ++it) {
        std::string scope = std::get<0>(it->first);
        unsigned int depth = std::get<1>(it->first);
        unsigned int label = std::get<2>(it->first);
        unsigned int return_label = std::get<3>(it->first);
        str << "\t\t";
        str << "(" << scope << ", " << depth << ", " << label << ", " << return_label << ")";
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

bool Merger::reachedMergePoint(const Context &context) const {
    const Frame &frame = context.getFrame();
    unsigned int return_label = frame.getReturnLabel();
    std::string scope = frame.getScope();
    unsigned int depth = context.getCallStackDepth();
    const State &state = context.getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    return _merge_points.count(std::tuple(scope, depth, label, return_label)) != 0;
}

void Merger::push(std::unique_ptr<Context> context) {
    const Frame &frame = context->getFrame();
    unsigned int return_label = frame.getReturnLabel();
    std::string scope = frame.getScope();
    unsigned int depth = context->getCallStackDepth();
    const State &state = context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    assert(_merge_points.count(std::tuple(scope, depth, label, return_label)) != 0);
    assert(_merge_point_to_contexts.count(std::tuple(scope, depth, label, return_label)) != 0);
    _merge_point_to_contexts.at(std::tuple(scope, depth, label, return_label)).push_back(std::move(context));
}

std::unique_ptr<Context> Merger::merge() {
    auto logger = spdlog::get("Oa");
    auto it = std::find_if(_merge_point_to_contexts.begin(), _merge_point_to_contexts.end(),
                           [](const auto &merge_point_to_context) { return !merge_point_to_context.second.empty(); });
    assert(it != _merge_point_to_contexts.end());
    merge_point_t eligible_merge_point = it->first;
    for (const auto &merge_point_to_contexts : _merge_point_to_contexts) {
        merge_point_t merge_point = merge_point_to_contexts.first;
        if (merge_point_to_contexts.second.empty()) {
            // XXX only consider non-empty merge points
            continue;
        }
        unsigned int depth = std::get<1>(merge_point);
        unsigned int label = std::get<2>(merge_point);
        unsigned int return_label = std::get<3>(merge_point);
        if (std::get<1>(eligible_merge_point) < depth) {
            eligible_merge_point = merge_point;
        } else if (std::get<1>(eligible_merge_point) == depth && std::get<3>(eligible_merge_point) > return_label) {
            eligible_merge_point = merge_point;
        } else if (std::get<1>(eligible_merge_point) == depth && std::get<3>(eligible_merge_point) == return_label &&
                   std::get<2>(eligible_merge_point) > label) {
            // XXX always prioritize lower labels at same depth
            eligible_merge_point = merge_point;
        }
    }
    std::vector<std::unique_ptr<Context>> contexts = std::move(_merge_point_to_contexts.at(eligible_merge_point));
    std::string scope = std::get<0>(eligible_merge_point);
    unsigned int depth = std::get<1>(eligible_merge_point);
    unsigned int label = std::get<2>(eligible_merge_point);
    unsigned int return_label = std::get<3>(eligible_merge_point);
    SPDLOG_LOGGER_TRACE(logger, "Trying to merge contexts at merge point: ({}, {}, {}, {})", scope, depth, label,
                        return_label);
    std::unique_ptr<Context> merged_context = std::move(contexts.at(0));
    contexts.erase(contexts.begin());
    while (!contexts.empty()) {
        std::unique_ptr<Context> context = std::move(contexts.at(0));
        contexts.erase(contexts.begin());
        merged_context = merge(std::move(merged_context), std::move(context));
    }
    return merged_context;
}

std::unique_ptr<Context> Merger::merge(std::unique_ptr<Context> context_1, std::unique_ptr<Context> context_2) {
    auto logger = spdlog::get("Oa");
    SPDLOG_LOGGER_TRACE(logger, "Merging...\n{}\n{}", *context_1, *context_2);

    unsigned int cycle_1 = context_1->getCycle();
    unsigned int cycle_2 = context_2->getCycle();
    assert(cycle_1 == cycle_2);
    unsigned int cycle = context_1->getCycle();

    State &state_1 = context_1->getState();
    State &state_2 = context_2->getState();
    const Vertex &vertex_1 = state_1.getVertex();
    const Vertex &vertex_2 = state_2.getVertex();
    assert(vertex_1.getLabel() == vertex_2.getLabel());
    const Vertex &vertex = vertex_1;

    const std::deque<std::shared_ptr<Frame>> &call_stack_1 = context_1->getCallStack();
    const std::deque<std::shared_ptr<Frame>> &call_stack_2 = context_2->getCallStack();
    assert(context_1->getCallStackDepth() == context_2->getCallStackDepth());
    for (std::deque<std::shared_ptr<Frame>>::size_type i = 0; i < context_1->getCallStackDepth(); ++i) {
        const Frame &frame_1 = *call_stack_1.at(i);
        const Frame &frame_2 = *call_stack_2.at(i);
        assert(frame_1.getScope() == frame_2.getScope() && frame_1.getReturnLabel() == frame_2.getReturnLabel());
    }

    z3::expr_vector path_constraint_1(_solver->getContext());
    for (const z3::expr &constraint_1 : state_1.getPathConstraint()) {
        path_constraint_1.push_back(constraint_1);
    }
    z3::expr_vector path_constraint_2(_solver->getContext());
    for (const z3::expr &constraint_2 : state_2.getPathConstraint()) {
        path_constraint_2.push_back(constraint_2);
    }
    std::vector<z3::expr> path_constraint{(z3::mk_and(path_constraint_1) || z3::mk_and(path_constraint_2)).simplify()};

    // XXX due to the "global" versioning and therefore the resulting implicit SSA-form, the prefix of both states
    // XXX must always be the same, i.e., the valuations of variables with the same version are structurally
    // XXX equivalent. Augment the symbolic valuations with the mutual exclusive versions and merge on the highest
    // XXX version of each valuation in case they differ.
    std::map<std::string, z3::expr> symbolic_valuations = state_1.getSymbolicValuations();
    for (const auto &symbolic_valuation_2 : state_2.getSymbolicValuations()) {
        std::string contextualized_name = symbolic_valuation_2.first;
        auto it = symbolic_valuations.find(contextualized_name);
        if (it == symbolic_valuations.end()) {
            symbolic_valuations.emplace(symbolic_valuation_2);
        } else {
            z3::expr symbolic_expression_1 = state_1.getSymbolicValuation(contextualized_name);
            z3::expr symbolic_expression_2 = state_2.getSymbolicValuation(contextualized_name);
            assert(z3::eq(symbolic_expression_1, symbolic_expression_2));
            SPDLOG_LOGGER_TRACE(logger,
                                "Valuation \"{}\" maps to structurally equivalent symbolic expressions {} and {} in "
                                "both contexts, no merge required.",
                                contextualized_name, symbolic_expression_1, symbolic_expression_2);
        }
    }
    // XXX merge inside a call frame requires merge on all variables as the prefix of both contexts could be different
    const Frame &main_frame = context_1->getMainFrame();
    const Cfg &main_cfg = main_frame.getCfg();
    std::map<std::string, unsigned int> flattened_name_to_version;
    for (auto it = main_cfg.flattenedInterfaceBegin(); it != main_cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        // XXX concrete and symbolic store are invariant w.r.t. variable versioning
        unsigned int highest_symbolic_version_1 = state_1.getHighestVersion(flattened_name);
        unsigned int highest_symbolic_version_2 = state_2.getHighestVersion(flattened_name);
        if (highest_symbolic_version_1 != highest_symbolic_version_2) {
            std::string contextualized_name_1 =
                    flattened_name + "_" + std::to_string(highest_symbolic_version_1) + "__" + std::to_string(cycle);
            std::string contextualized_name_2 =
                    flattened_name + "_" + std::to_string(highest_symbolic_version_2) + "__" + std::to_string(cycle);
            SPDLOG_LOGGER_TRACE(logger, "Merging on variable \"{}\" with versions {} and {}.", flattened_name,
                                contextualized_name_1, contextualized_name_2);

            z3::expr symbolic_expression_1 = state_1.getSymbolicValuation(contextualized_name_1);
            z3::expr symbolic_expression_2 = state_2.getSymbolicValuation(contextualized_name_2);

            // XXX introduce a fresh new variable as lhs of the merge, depending on where and when the merge occurs,
            // XXX the maximum version + 1 of the two contexts might not be the highest version, hence retrieve the
            // XXX maximum version from the global version store of the executor and increment it
            unsigned int version = _executor->getVersion(flattened_name) + 1;
            // update version globally
            _executor->setVersion(flattened_name, version);
            // update version locally
            flattened_name_to_version.emplace(flattened_name, version);
            std::string contextualized_name =
                    flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);

            if (z3::eq(symbolic_expression_1, symbolic_expression_2)) {
                // XXX structurally equivalent, refrain from using ite and replace it by either symbolic expression
                symbolic_valuations.emplace(contextualized_name, symbolic_expression_1);
            } else {
                const ir::DataType &data_type = it->getDataType();
                z3::expr symbolic_variable_1 = _solver->makeConstant(contextualized_name_1, data_type);
                z3::expr symbolic_variable_2 = _solver->makeConstant(contextualized_name_2, data_type);
                z3::expr symbolic_expression =
                        z3::ite(z3::mk_and(path_constraint_1), symbolic_variable_1, symbolic_variable_2).simplify();
                symbolic_valuations.emplace(contextualized_name, symbolic_expression);
            }
        } else {
            assert(highest_symbolic_version_1 == highest_symbolic_version_2);
            flattened_name_to_version.emplace(flattened_name, highest_symbolic_version_1);
        }
    }
    // merged state
    std::unique_ptr<State> state = std::make_unique<State>(
            vertex, std::move(symbolic_valuations), std::move(path_constraint), std::move(flattened_name_to_version));
    // XXX copy of call stack
    std::deque<std::shared_ptr<Frame>> call_stack = call_stack_1;
    std::unique_ptr<Context> context = std::make_unique<Context>(cycle, std::move(state), std::move(call_stack));
    return context;
}

void Merger::initialize(const Cfg &cfg) {
    // collect potential locations for merging
    std::string scope = cfg.getName();
    std::set<std::string> visited_cfgs;
    initializeMergePoints(cfg, scope, 1, cfg.getEntryLabel(), visited_cfgs);
    // initialize context queues
    for (const auto &merge_point : _merge_points) {
        _merge_point_to_contexts.emplace(merge_point, std::vector<std::unique_ptr<Context>>());
    }
}

void Merger::initializeMergePoints(const Cfg &cfg, const std::string &scope, unsigned int depth,
                                   unsigned int return_label, std::set<std::string> &visited_cfgs) {
    std::string name = cfg.getName();
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        switch (it->getType()) {
            case Vertex::Type::ENTRY: {
                break;
            }
            case Vertex::Type::REGULAR: {
                unsigned int label = it->getLabel();
                std::vector<std::shared_ptr<Edge>> incoming_edges = cfg.getIncomingEdges(label);
                if (incoming_edges.size() > 1) {
                    bool eligible_merge_point = true;
                    for (const auto &incoming_edge : incoming_edges) {
                        if ((incoming_edge->getType() == Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN) ||
                            ((incoming_edge->getType() == Edge::Type::INTERPROCEDURAL_CALL)) ||
                            ((incoming_edge->getType() == Edge::Type::INTERPROCEDURAL_RETURN))) {
                            eligible_merge_point = false;
                        }
                    }
                    if (eligible_merge_point) {
                        _merge_points.emplace(scope, depth, label, return_label);
                    }
                }
                if (auto call_instruction = dynamic_cast<const ir::CallInstruction *>(it->getInstruction())) {
                    std::string callee_scope = scope + "." + call_instruction->getVariableAccess().getName();
                    std::shared_ptr<Cfg> callee = cfg.getCallee(label);
                    const auto &edge = cfg.getIntraproceduralCallToReturnEdge(label);
                    initializeMergePoints(*callee, callee_scope, depth + 1, edge.getTargetLabel(), visited_cfgs);
                }
                break;
            }
            case Vertex::Type::EXIT: {
                _merge_points.emplace(scope, depth, cfg.getExitLabel(), return_label);
                break;
            }
        }
    }
}