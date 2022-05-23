#ifndef AHORN_BASELINE_EXPLORER_H
#define AHORN_BASELINE_EXPLORER_H

#include "cfg/cfg.h"
#include "se/baseline/context/context.h"

#include <memory>
#include <vector>

namespace se::baseline {
    class Explorer {
    private:
        friend class Engine;

    public:
        Explorer();

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Explorer &explorer) {
            return explorer.print(os);
        }

        bool isEmpty() const;

        void pushContext(std::unique_ptr<Context> context);

        std::unique_ptr<Context> popContext();

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
}// namespace se::baseline

#endif//AHORN_BASELINE_EXPLORER_H
