#ifndef AHORN_CBMC_ENGINE_H
#define AHORN_CBMC_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/cbmc/context/context.h"
#include "se/cbmc/execution/executor.h"
#include "se/cbmc/exploration/explorer.h"
#include "se/cbmc/merger.h"
#include "se/cbmc/z3/solver.h"
#include "se/configuration.h"

#include <chrono>
#include <memory>
#include <vector>

namespace se::cbmc {
    class Engine {
    public:
        // XXX default constructor disabled
        Engine() = delete;
        // XXX copy constructor disabled
        Engine(const Engine &other) = delete;
        // XXX copy assignment disabled
        Engine &operator=(const Engine &) = delete;

        explicit Engine(std::unique_ptr<Configuration> configuration);

        void run(const Cfg &cfg);

    private:
        enum TerminationCriteria { TIME_OUT = 1 << 0, CYCLE_BOUND = 1 << 1, COVERAGE = 1 << 2 };

        friend inline TerminationCriteria operator|(TerminationCriteria tc1, TerminationCriteria tc2) {
            return static_cast<TerminationCriteria>(static_cast<int>(tc1) | static_cast<int>(tc2));
        }

        bool isTerminationCriteriaMet() const;

        void initialize(const Cfg &cfg);

        std::unique_ptr<Context> step();

    private:
        std::unique_ptr<Configuration> _configuration;
        unsigned int _cycle;
        std::unique_ptr<Solver> _solver;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<Executor> _executor;
        std::unique_ptr<Merger> _merger;

        TerminationCriteria _termination_criteria;

        // configurable parameter cycle bound
        unsigned int _cycle_bound = 5;

        std::chrono::time_point<std::chrono::system_clock> _begin_time_point;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_ENGINE_H
