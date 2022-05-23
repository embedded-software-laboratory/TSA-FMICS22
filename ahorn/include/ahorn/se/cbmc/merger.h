#ifndef AHORN_CBMC_MERGER_H
#define AHORN_CBMC_MERGER_H

#include "cfg/cfg.h"
#include "se/cbmc/context/context.h"
#include "se/cbmc/z3/solver.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace se::cbmc {
    class Merger {
    private:
        friend class Engine;

    public:
        // XXX default constructor disabled
        Merger() = delete;
        // XXX copy constructor disabled
        Merger(const Merger &other) = delete;
        // XXX copy assignment disabled
        Merger &operator=(const Merger &) = delete;

        explicit Merger(Solver &solver);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Merger &merger) {
            return merger.print(os);
        }

        bool isEmpty() const;

        bool reachedMergePoint(const Context &context) const;

        void pushContext(std::unique_ptr<Context> context);

        std::unique_ptr<Context> merge();

    private:
        std::unique_ptr<Context> merge(std::unique_ptr<Context> context_1, std::unique_ptr<Context> context_2);

        void mergeVariable(const std::string &merged_contextualized_name, const State &state,
                           const std::string &contextualized_name_to_merge,
                           std::map<std::string, std::map<std::string, z3::expr>> &modified_variable_instances);

        void initialize(const Cfg &cfg);

        void initializeMergePoints(const Cfg &cfg, const std::string &scope, unsigned int depth,
                                   unsigned int return_label, std::set<std::string> &visited_cfgs);

    private:
        Solver *const _solver;
        // enforce only realizable paths, i.e., merge point consists of scope, depth, label, and return label
        using merge_point_t = std::tuple<std::string, unsigned int, unsigned int, unsigned int>;
        std::set<merge_point_t> _merge_points;
        std::map<merge_point_t, std::vector<std::unique_ptr<Context>>> _merge_point_to_contexts;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_MERGER_H
