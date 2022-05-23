#ifndef AHORN_EXPLORER_H
#define AHORN_EXPLORER_H

#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"

#include <memory>
#include <set>
#include <vector>

namespace se {
    class Explorer {
    private:
        friend class Engine;

    public:
        // XXX default constructor disabled
        Explorer() = delete;
        // XXX copy constructor disabled
        Explorer(const Explorer &other) = delete;
        // XXX copy assignment disabled
        Explorer &operator=(const Explorer &) = delete;

        explicit Explorer(const Configuration &configuration);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Explorer &explorer) {
            return explorer.print(os);
        }

        bool isEmpty() const;

        std::unique_ptr<Context> popContext();

        void pushContext(std::unique_ptr<Context> context);

        std::pair<bool, bool> updateCoverage(unsigned int label, const Context &context);

    private:
        void initialize(const Cfg &cfg);

        void initializeCoverage(const Cfg &cfg, std::set<std::string> &visited_cfgs);

    private:
        const Configuration &_configuration;

        std::vector<std::unique_ptr<Context>> _contexts;

        double _statement_coverage;

        // TODO (05.01.2022): Add the scope for easier debugging.
        std::map<unsigned int, bool> _covered_statements;

        double _branch_coverage;

        std::map<unsigned int, std::pair<bool, bool>> _covered_branches;
    };
}// namespace se

#endif//AHORN_EXPLORER_H
