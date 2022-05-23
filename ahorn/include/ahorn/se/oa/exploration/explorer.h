#ifndef AHORN_OA_EXPLORER_H
#define AHORN_OA_EXPLORER_H

#include "cfg/cfg.h"
#include "se/oa/context/context.h"

#include <map>
#include <memory>
#include <vector>

namespace se::oa {
    class Explorer {
    private:
        friend class Engine;

    public:
        Explorer();
        // XXX copy constructor disabled
        Explorer(const Explorer &other) = delete;
        // XXX copy assignment disabled
        Explorer &operator=(const Explorer &) = delete;

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Explorer &explorer) {
            return explorer.print(os);
        }

        bool isEmpty() const;

        void push(std::unique_ptr<Context> context);

        std::unique_ptr<Context> pop();

        std::pair<bool, bool> updateCoverage(unsigned int label, const Context &context);

    private:
        void initialize(const Cfg &cfg);

        void initializeCoverage(const Cfg &cfg, std::set<std::string> &visited_cfgs);

    private:
        std::vector<std::unique_ptr<Context>> _contexts;
        double _statement_coverage;
        std::map<unsigned int, bool> _covered_statements;
        double _branch_coverage;
        std::map<unsigned int, std::pair<bool, bool>> _covered_branches;
    };
}// namespace se::oa

#endif
