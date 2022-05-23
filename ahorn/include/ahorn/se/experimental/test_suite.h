#ifndef AHORN_TEST_SUITE_H
#define AHORN_TEST_SUITE_H

#include "se/experimental/context/context.h"
#include "se/experimental/test_case.h"
#include "se/experimental/z3/manager.h"

#include <memory>
#include <vector>

namespace se {
    class TestSuite {
    public:
        // XXX default constructor disabled
        TestSuite() = delete;
        // XXX copy constructor disabled
        TestSuite(const TestSuite &other) = delete;
        // XXX copy assignment disabled
        TestSuite &operator=(const TestSuite &) = delete;

        explicit TestSuite(Manager &manager);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const TestSuite &test_suite) {
            return test_suite.print(os);
        }

        void toXML(const std::string &path = std::string()) const;

        void toXML(const TestCase &test_case, const std::string &path) const;

        std::unique_ptr<TestCase> fromXML(const std::string &path) const;

        const std::vector<std::unique_ptr<TestCase>> &getTestCases() const;

        void createTestCase(const Context &context);

    private:
        Manager *const _manager;

        std::vector<std::unique_ptr<TestCase>> _test_cases;
    };
}// namespace se

#endif//AHORN_TEST_SUITE_H
