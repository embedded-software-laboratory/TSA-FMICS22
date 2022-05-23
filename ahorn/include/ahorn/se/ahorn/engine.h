#ifndef AHORN_AHORN_ENGINE_H
#define AHORN_AHORN_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/ahorn/execution/executor.h"
#include "se/ahorn/exploration/explorer.h"
#include "se/ahorn/merger.h"
#include "se/ahorn/test/test_suite.h"
#include "se/ahorn/z3/solver.h"
#include "se/configuration.h"

#include "boost/optional.hpp"

#include <chrono>
#include <memory>

class TestLibSeAhorn_Call_Coverage_In_3_Cycles_Test;

namespace se::ahorn {
    class Engine {
    private:
        FRIEND_TEST(::TestLibSeAhorn, Call_Coverage_In_3_Cycles);

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

        boost::optional<TerminationCriteria> isLocalTerminationCriteriaMet() const;

        bool isTimeOut() const;

        void initialize(const Cfg &cfg);

        std::pair<std::unique_ptr<Context>, boost::optional<TerminationCriteria>> step();

    private:
        std::unique_ptr<Configuration> _configuration;
        unsigned int _cycle;
        std::unique_ptr<Solver> _solver;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<Executor> _executor;
        std::unique_ptr<Merger> _merger;
        std::unique_ptr<TestSuite> _test_suite;

        TerminationCriteria _termination_criteria;

        std::chrono::time_point<std::chrono::system_clock> _begin_time_point;

        // configurable parameter time out in ms resolution
        long _time_out = 10000;

        // configurable parameter cycle bound
        unsigned int _cycle_bound = 10;
    };
}// namespace se::ahorn

#endif//AHORN_AHORN_ENGINE_H
