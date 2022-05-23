#ifndef AHORN_ENGINE_H
#define AHORN_ENGINE_H

#include <gtest/gtest_prod.h>

#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"
#include "se/experimental/executor.h"
#include "se/experimental/explorer.h"
#include "se/experimental/merger.h"
#include "se/experimental/test_suite.h"
#include "se/experimental/z3/manager.h"

#include <chrono>
#include <memory>
#include <vector>

class TestLibSe_Engine_If_Cfg_Test;
class TestLibSe_Engine_Constant_Propagation_Cfg_Test;
class TestLibSe_Engine_Constant_Propagation_With_Input_Cfg_Test;
class TestLibSe_Engine_Constant_Propagation_With_Shadows_Cfg_Test;
class TestLibSe_Engine_Call_Cfg_Test;
class TestLibSe_Engine_Call_Two_Times_Cfg_Test;
class TestLibSe_Engine_Call_With_Tripple_Diamond_Cfg_Test;
class TestLibSe_Engine_While_Cfg_Test;
class TestLibSe_Engine_Non_deterministic_If_Cfg_Test;
class TestLibSe_Engine_While_With_If_Cfg_Test;
class TestLibSe_Engine_Branch_Coverage_In_3_Cycles_Cfg_Test;
class TestLibSe_Engine_Call_Coverage_In_3_Cycles_Cfg_Test;
class TestLibSe_Engine_Call_Coverage_In_1_Cycle_With_Change_Shadow_Cfg_Test;
class TestLibSe_Engine_Breadth_First_and_Merging_Cfg_Test;
class TestLibSe_Engine_Antivalent_Test_Cfg_Test;
class TestLibSe_Engine_Emergency_Stop_Test_Cfg_Test;
class TestLibSe_Engine_If_Change_Cfg_Test;
class TestLibSe_Engine_If_Reaching_Change_Cfg_Test;
class TestLibSe_Engine_Merge_If_Call_Cfg_Test;
class TestLibSe_Engine_Different_Frame_Stacks_Cfg_Test;
class TestLibSe_Engine_Mode_Selector_Test_Cfg_Test;

namespace se {
    class Engine {
    private:
        FRIEND_TEST(::TestLibSe, Engine_If_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Constant_Propagation_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Constant_Propagation_With_Input_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Constant_Propagation_With_Shadows_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Call_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Call_Two_Times_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Call_With_Tripple_Diamond_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_While_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Non_deterministic_If_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_While_With_If_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Branch_Coverage_In_3_Cycles_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Call_Coverage_In_3_Cycles_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Call_Coverage_In_1_Cycle_With_Change_Shadow_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Breadth_First_and_Merging_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Antivalent_Test_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Emergency_Stop_Test_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_If_Change_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_If_Reaching_Change_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Merge_If_Call_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Different_Frame_Stacks_Cfg);
        FRIEND_TEST(::TestLibSe, Engine_Mode_Selector_Test_Cfg);

    public:
        enum class EngineStatus { EXPECTED_BEHAVIOR, DIVERGENT_BEHAVIOR, POTENTIAL_DIVERGENT_BEHAVIOR };

        // XXX default constructor disabled
        Engine() = delete;
        // XXX copy constructor disabled
        Engine(const Engine &other) = delete;
        // XXX copy assignment disabled
        Engine &operator=(const Engine &) = delete;

        explicit Engine(std::unique_ptr<Configuration> configuration);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Engine &engine) {
            return engine.print(os);
        }

        void run(const Cfg &cfg);

    private:
        void initialize(const Cfg &cfg) const;

        std::unique_ptr<Context> createInitialContext(const Cfg &cfg) const;

        std::unique_ptr<Context> translateDivergentContext(z3::context &z3_context,
                                                           std::unique_ptr<Context> context) const;

        z3::expr substituteShadowExpression(
                const std::map<std::string, std::shared_ptr<ShadowExpression>> &shadow_name_to_shadow_expression,
                const z3::expr &z3_expression) const;

        std::pair<EngineStatus, std::vector<std::unique_ptr<Context>>> step();

    private:
        std::unique_ptr<Configuration> _configuration;

        EngineStatus _engine_status;

        unsigned int _cycle;

        std::unique_ptr<Manager> _manager;

        std::unique_ptr<Explorer> _explorer;

        std::unique_ptr<Executor> _executor;

        std::unique_ptr<Merger> _merger;

        std::unique_ptr<TestSuite> _test_suite;

        std::chrono::time_point<std::chrono::system_clock> _begin_time_point;
    };
}// namespace se

#endif//AHORN_ENGINE_H
