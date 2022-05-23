#ifndef AHORN_MERGER_H
#define AHORN_MERGER_H

#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"
#include "se/experimental/z3/manager.h"

#include "boost/optional.hpp"

#include <memory>
#include <vector>

namespace se {
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

        Merger(const Configuration &configuration, Manager &manager);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Merger &merger) {
            return merger.print(os);
        }

        bool isEmpty() const;

        void pushContext(std::unique_ptr<Context> context);

        bool reachedMergePoint(const Context &context) const;

        std::unique_ptr<Context> merge();

    private:
        std::unique_ptr<Context> merge(std::unique_ptr<Context> context_1, std::unique_ptr<Context> context_2);

        std::deque<std::shared_ptr<Frame>> mergeFrameStack(const Context &context_1, const Context &context_2);

        void initialize(const Cfg &cfg);

        void initializeMergePoints(const Cfg &cfg, unsigned int depth, std::string scope, unsigned int return_label,
                                   std::set<std::string> &visited_cfgs);

    private:
        const Configuration &_configuration;

        Manager *const _manager;

        std::vector<std::unique_ptr<Context>> _contexts;

        // a merge point consists of a scope and a label
        std::set<std::tuple<std::string, unsigned int, unsigned int>> _merge_points;

        std::map<std::tuple<std::string, unsigned int, unsigned int>, std::vector<std::unique_ptr<Context>>>
                _merge_point_to_contexts;
    };
}// namespace se

#endif//AHORN_MERGER_H
