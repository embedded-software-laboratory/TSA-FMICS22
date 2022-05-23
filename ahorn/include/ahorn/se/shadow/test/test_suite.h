#ifndef AHORN_SHADOW_TEST_SUITE_H
#define AHORN_SHADOW_TEST_SUITE_H

#include "se/shadow/context/context.h"
#include "se/shadow/test/test_case.h"
#include "se/shadow/z3/solver.h"

#include <memory>
#include <vector>

namespace se::shadow {
    class TestSuite {
    private:
        friend class Engine;

    public:
        // XXX default constructor disabled
        TestSuite() = delete;
        // XXX copy constructor disabled
        TestSuite(const TestSuite &other) = delete;
        // XXX copy assignment disabled
        TestSuite &operator=(const TestSuite &) = delete;

        explicit TestSuite(Solver &solver);

        const std::vector<std::shared_ptr<TestCase>> &getTestCases() const;

        std::shared_ptr<TestCase> deriveTestCase(const Context &context);

        void toXML(const std::string &path = std::string()) const;

        void fromXML(const std::string &path);

    private:
        void reset();

        void toXML(const TestCase &test_case, const std::string &path) const;

        std::unique_ptr<TestCase> buildFromXML(const std::string &path) const;

    private:
        Solver *const _solver;
        std::vector<std::shared_ptr<TestCase>> _test_cases;
    };
}// namespace se::shadow

#endif//AHORN_SHADOW_TEST_SUITE_H
