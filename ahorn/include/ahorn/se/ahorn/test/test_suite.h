#ifndef AHORN_AHORN_TEST_SUITE_H
#define AHORN_AHORN_TEST_SUITE_H

#include "se/ahorn/context/context.h"
#include "se/ahorn/test/test_case.h"
#include "se/ahorn/z3/solver.h"

#include <memory>
#include <vector>

namespace se::ahorn {
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

        void deriveTestCase(const Context &context);

        void toXML(const std::string &path = std::string()) const;

        void fromXML(const std::string &path);

    private:
        void toXML(const TestCase &test_case, const std::string &path) const;

        std::unique_ptr<TestCase> buildFromXML(const std::string &path) const;

    private:
        Solver *const _solver;

        std::vector<std::unique_ptr<TestCase>> _test_cases;
    };
}// namespace se::ahorn

#endif//AHORN_AHORN_TEST_SUITE_H
