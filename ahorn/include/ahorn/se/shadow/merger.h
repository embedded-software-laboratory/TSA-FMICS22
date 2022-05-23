#ifndef AHORN_SHADOW_MERGER_H
#define AHORN_SHADOW_MERGER_H

#include "cfg/cfg.h"
#include "se/shadow/context/context.h"
#include "se/shadow/execution/executor.h"
#include "se/shadow/z3/solver.h"

#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>

namespace se::shadow {
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

        Merger(Solver &solver, Executor &executor);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Merger &merger) {
            return merger.print(os);
        }

        bool isEmpty() const;

        bool reachedMergePoint(const Context &context) const;

        void push(std::unique_ptr<Context> context);

        std::unique_ptr<Context> merge();

    private:
        std::unique_ptr<Context> merge(std::unique_ptr<Context> context_1, std::unique_ptr<Context> context_2);

        void initialize(const Cfg &cfg);

        void initializeMergePoints(const Cfg &cfg, const std::string &scope, unsigned int depth,
                                   unsigned int return_label, std::set<std::string> &visited_cfgs);

        void reset();

    private:
        Solver *const _solver;
        Executor *const _executor;
        // enforce only realizable paths, i.e., merge point consists of scope, depth, label, and return label
        using merge_point_t = std::tuple<std::string, unsigned int, unsigned int, unsigned int>;
        std::set<merge_point_t> _merge_points;
        std::map<merge_point_t, std::vector<std::unique_ptr<Context>>> _merge_point_to_contexts;
    };
}// namespace se::shadow

#endif//AHORN_SHADOW_MERGER_H
