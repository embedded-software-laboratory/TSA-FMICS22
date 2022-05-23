#ifndef AHORN_CBMC_EXPLORER_H
#define AHORN_CBMC_EXPLORER_H

#include "cfg/cfg.h"
#include "se/cbmc/context/context.h"
#include "se/cbmc/z3/solver.h"

#include <memory>
#include <vector>

namespace se::cbmc {
    class Explorer {
    private:
        friend class Engine;

    public:
        Explorer() = default;

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Explorer &explorer) {
            return explorer.print(os);
        }

        bool isEmpty() const;

        void push(std::unique_ptr<Context> context);

        std::unique_ptr<Context> pop();

        bool hasGoal(const std::string &goal);

        void removeGoal(const std::string &goal);

        void checkGoals(Solver &solver, const Context &context);

    private:
        void initialize(const Cfg &cfg);

        void initializeGoals(const Cfg &cfg, const std::string &scope, std::set<std::string> &visited_cfgs);

    private:
        std::vector<std::unique_ptr<Context>> _contexts;
        std::set<std::string> _goals;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_EXPLORER_H
