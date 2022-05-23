#ifndef AHORN_SHADOW_EXPLORER_H
#define AHORN_SHADOW_EXPLORER_H

#include "cfg/cfg.h"
#include "se/shadow/context/context.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace se::shadow {
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

        void initialize(const Cfg &cfg, const Context &context,
                        const std::map<unsigned int,
                                       std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                             std::vector<unsigned int>>>> &execution_history);

        void initializeCoverage(const Cfg &cfg, std::set<std::string> &visited_cfgs);

        void reset();

    private:
        std::vector<std::unique_ptr<Context>> _contexts;
        double _statement_coverage;
        std::map<unsigned int, bool> _covered_statements;
        double _branch_coverage;
        std::map<unsigned int, std::pair<bool, bool>> _covered_branches;
    };
}// namespace se::shadow

#endif//AHORN_SHADOW_EXPLORER_H
