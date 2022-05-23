#ifndef AHORN_TEST_CASE_H
#define AHORN_TEST_CASE_H

#include "se/experimental/expression/concrete_expression.h"

#include "z3++.h"

#include <map>
#include <memory>

namespace se {
    class TestCase {
    public:
        // XXX default constructor disabled
        TestCase() = delete;
        // XXX copy constructor disabled
        TestCase(const TestCase &other) = delete;
        // XXX copy assignment disabled
        TestCase &operator=(const TestCase &) = delete;

        TestCase(std::map<unsigned int, std::map<std::string, z3::expr>> inputs,
                 std::map<unsigned int, std::map<std::string, std::shared_ptr<ConcreteExpression>>> outputs);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const TestCase &test_case) {
            return test_case.print(os);
        }

        const std::map<unsigned int, std::map<std::string, z3::expr>> &getInputs() const;

    private:
        std::map<unsigned int, std::map<std::string, z3::expr>> _inputs;
        std::map<unsigned int, std::map<std::string, std::shared_ptr<ConcreteExpression>>> _outputs;
    };
}// namespace se

#endif//AHORN_TEST_CASE_H
