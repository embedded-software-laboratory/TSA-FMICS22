#ifndef AHORN_SIMULATOR_EXPLORER_H
#define AHORN_SIMULATOR_EXPLORER_H

#include "cfg/cfg.h"
#include "se/simulator/context/context.h"
#include "se/shadow/engine.h"

#include <map>
#include <set>

namespace se::simulator {
    class Explorer {
    private:
        friend class Engine;
        friend class se::shadow::Engine;

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

        std::pair<bool, bool> updateCoverage(unsigned int label, const Context &context);

    private:
        void initialize(const Cfg &cfg);

        void initializeCoverage(const Cfg &cfg, std::set<std::string> &visited_cfgs);

    private:
        double _statement_coverage;
        std::map<unsigned int, bool> _covered_statements;
        double _branch_coverage;
        std::map<unsigned int, std::pair<bool, bool>> _covered_branches;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_EXPLORER_H
