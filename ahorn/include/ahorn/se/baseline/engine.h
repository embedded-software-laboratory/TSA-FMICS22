#ifndef AHORN_BASELINE_ENGINE_H
#define AHORN_BASELINE_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/baseline/execution/executor.h"
#include "se/baseline/exploration/explorer.h"
#include "se/baseline/z3/solver.h"

#include <memory>

class TestLibSeBaseline_Call_Coverage_In_3_Cycles_Cfg_Test;
class TestLibSeBaseline_Benchmark_Antivalent_Test;

namespace se::baseline {
    class Engine {
    private:
        FRIEND_TEST(::TestLibSeBaseline, Call_Coverage_In_3_Cycles_Cfg);
        FRIEND_TEST(::TestLibSeBaseline, Benchmark_Antivalent);

    public:
        Engine();

        void run();

    private:
        void initialize(const Cfg &cfg);

        std::vector<std::unique_ptr<Context>> step();

    private:
        unsigned int _cycle;
        std::unique_ptr<Solver> _solver;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<Executor> _executor;
    };
}// namespace se::baseline

#endif//AHORN_BASELINE_ENGINE_H
