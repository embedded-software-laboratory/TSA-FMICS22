#ifndef AHORN_SIMULATOR_ENGINE_H
#define AHORN_SIMULATOR_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/shadow/test/test_case.h"
#include "se/shadow/z3/solver.h"
#include "se/simulator/context/context.h"
#include "se/simulator/execution/executor.h"
#include "se/simulator/exploration/explorer.h"

#include "z3++.h"

#include <map>
#include <tuple>
#include <vector>

class TestLibSeShadow_Call_Coverage_In_1_Cycle_With_Change_Shadow_Test;
class TestLibSeSimulator_Scenario3_Final_Test;

namespace se::simulator {
    class Engine {
    private:
        friend class se::shadow::Engine;

    private:
        FRIEND_TEST(::TestLibSeShadow, Call_Coverage_In_1_Cycle_With_Change_Shadow);
        FRIEND_TEST(::TestLibSeSimulator, Scenario3_Final);

    public:
        // XXX default constructor disabled
        Engine() = delete;
        // XXX copy constructor disabled
        Engine(const Engine &other) = delete;
        // XXX copy assignment disabled
        Engine &operator=(const Engine &) = delete;

        Engine(Evaluator::ShadowProcessingMode shadow_processing_mode, shadow::Solver &solver);

        std::pair<
                std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                             std::vector<unsigned int>>>>,
                std::map<unsigned int, std::map<std::string, z3::expr>>>
        run(const Cfg &cfg, const std::map<std::string, z3::expr> &initial_concrete_state_valuations,
            const std::map<unsigned int, std::map<std::string, z3::expr>> &cycle_to_concrete_input_valuations);

        void run(const Cfg &cfg, const std::vector<std::shared_ptr<se::shadow::TestCase>> &test_cases);

    private:
        void initialize(const Cfg &cfg);

        std::unique_ptr<Context>
        initializeFromConcreteValuation(const Cfg &cfg,
                                        const std::map<std::string, z3::expr> &initial_concrete_state_valuations);

        void reset();

        void step(Context &context);

        void
        step(Context &context,
             std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                          std::vector<unsigned int>>>> &execution_history);

    private:
        unsigned int _cycle;
        shadow::Solver *const _solver;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<Executor> _executor;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_ENGINE_H
