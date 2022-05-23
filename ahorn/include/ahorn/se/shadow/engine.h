#ifndef AHORN_SHADOW_ENGINE_H
#define AHORN_SHADOW_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/configuration.h"
#include "se/shadow/execution/divergence_executor.h"
#include "se/shadow/execution/executor.h"
#include "se/shadow/exploration/explorer.h"
#include "se/shadow/merger.h"
#include "se/shadow/test/test_suite.h"
#include "se/shadow/z3/solver.h"

#include "boost/optional.hpp"

#include <chrono>

class TestLibSeShadow_Call_Coverage_In_1_Cycle_With_Change_Shadow_Test;
class TestLibSeSimulator_Scenario3_Final_Test;

namespace se::shadow {
    class Engine {
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

        explicit Engine(std::unique_ptr<Configuration> configuration);

        void run(const Cfg &cfg, const std::string &path);

        std::vector<std::shared_ptr<TestCase>> collectChangeTraversingTestCases(const Cfg &cfg,
                                                                                const std::string &path);

        // C.f. Concolic phase (= Phase 1) of SSE in Kuchta et al.
        std::vector<std::pair<std::unique_ptr<Context>, std::shared_ptr<TestCase>>>
        findDivergentContexts(const Cfg &cfg, std::shared_ptr<TestCase> test_case);

        // C.f. Bounded symbolic execution (BSE, = Phase 2) of SSE in Kuchta et al.
        std::vector<std::shared_ptr<TestCase>>
        performBoundedExecution(const Cfg &cfg, std::unique_ptr<Context> divergent_context, const TestCase &test_case);

        std::vector<std::pair<std::shared_ptr<TestCase>, std::shared_ptr<TestCase>>>
        checkForOutputDifferences(const Cfg &cfg, const std::vector<std::shared_ptr<TestCase>> &divergent_test_cases);

    private:
        std::unique_ptr<Context> translateDivergentContext(std::unique_ptr<Context> divergent_context);

        enum TerminationCriteria { TIME_OUT = 1 << 0, CYCLE_BOUND = 1 << 1, COVERAGE = 1 << 2 };

        friend inline TerminationCriteria operator|(TerminationCriteria tc1, TerminationCriteria tc2) {
            return static_cast<TerminationCriteria>(static_cast<int>(tc1) | static_cast<int>(tc2));
        }

        bool isTerminationCriteriaMet() const;

        boost::optional<TerminationCriteria> isLocalTerminationCriteriaMet() const;

        bool isTimeOut() const;

        void initializeBoundedExecution(const Cfg &cfg, const Context &context, const TestCase &test_case);

        std::unique_ptr<Context> initializeDivergenceExecution(const Cfg &cfg, const TestCase &test_case);

        std::pair<std::unique_ptr<Context>, boost::optional<TerminationCriteria>>
        stepUntilBound(std::vector<std::shared_ptr<TestCase>> &test_cases);

        std::pair<DivergenceExecutor::ExecutionStatus, std::vector<std::unique_ptr<Context>>>
        stepUntilDivergence(Context &context);

        std::unique_ptr<TestCase> fromXML(const std::string &file_path);

        void reset();

    private:
        bool containsShadowExpression(const std::map<std::string, z3::expr> &valuations,
                                      const std::map<std::string, std::pair<z3::expr, z3::expr>> &shadow_valuations,
                                      const z3::expr &expression) const;

        z3::expr lowerToShadowExpression(const std::map<std::string, z3::expr> &valuations,
                                         const std::map<std::string, std::pair<z3::expr, z3::expr>> &shadow_valuations,
                                         const z3::expr &expression) const;

    private:
        std::unique_ptr<Configuration> _configuration;
        unsigned int _cycle;
        std::unique_ptr<Solver> _solver;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<DivergenceExecutor> _divergence_executor;
        std::unique_ptr<Executor> _executor;
        std::unique_ptr<Merger> _merger;
        std::unique_ptr<TestSuite> _test_suite;

        TerminationCriteria _termination_criteria;

        std::chrono::time_point<std::chrono::system_clock> _begin_time_point;

        // configurable parameter time out in ms resolution
        long _time_out = 10000;

        // configurable parameter cycle bound
        unsigned int _cycle_bound = 10;

        unsigned int _change_traversing_test_cases;
        unsigned int _change_annotated_labels;
        unsigned int _untouched_change_annotated_labels;
    };
}// namespace se::shadow

#endif//AHORN_SHADOW_ENGINE_H
