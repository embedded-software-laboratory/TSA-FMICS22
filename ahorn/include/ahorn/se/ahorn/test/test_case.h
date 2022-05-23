#ifndef AHORN_AHORN_TEST_CASE_H
#define AHORN_AHORN_TEST_CASE_H

#include "z3++.h"

#include <map>

namespace se::ahorn {
    class TestCase {
    public:
        // XXX default constructor disabled
        TestCase() = delete;
        // XXX copy constructor disabled
        TestCase(const TestCase &other) = delete;
        // XXX copy assignment disabled
        TestCase &operator=(const TestCase &) = delete;

        TestCase(std::map<std::string, z3::expr> initial_concrete_state_valuations,
                 std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_input_valuations);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const TestCase &test_case) {
            return test_case.print(os);
        }

        friend inline bool operator==(const TestCase &test_case_1, const TestCase &test_case_2) {
            const auto &initial_concrete_state_valuations_1 = test_case_1.getInitialConcreteStateValuations();
            const auto &initial_concrete_state_valuations_2 = test_case_2.getInitialConcreteStateValuations();
            assert(initial_concrete_state_valuations_1.size() == initial_concrete_state_valuations_2.size());
            for (const auto &initial_concrete_state_valuation_1 : initial_concrete_state_valuations_1) {
                const std::string &flattened_name = initial_concrete_state_valuation_1.first;
                if (!z3::eq(initial_concrete_state_valuation_1.second,
                            initial_concrete_state_valuations_2.at(flattened_name))) {
                    return false;
                }
            }
            const auto &cycle_to_concrete_input_valuations_1 = test_case_1.getCycleToConcreteInputValuations();
            const auto &cycle_to_concrete_input_valuations_2 = test_case_2.getCycleToConcreteInputValuations();
            if (cycle_to_concrete_input_valuations_1.size() == cycle_to_concrete_input_valuations_2.size()) {
                for (const auto &cycle_to_valuations_1 : cycle_to_concrete_input_valuations_1) {
                    unsigned int cycle = cycle_to_valuations_1.first;
                    for (const auto &valuation_1 : cycle_to_valuations_1.second) {
                        const std::string &flattened_name = valuation_1.first;
                        if (!z3::eq(valuation_1.second,
                                    cycle_to_concrete_input_valuations_2.at(cycle).at(flattened_name))) {
                            return false;
                        }
                    }
                }
                return true;
            } else {
                return false;
            }
        }

        const std::map<std::string, z3::expr> &getInitialConcreteStateValuations() const;

        const std::map<unsigned int, std::map<std::string, z3::expr>> &getCycleToConcreteInputValuations() const;

    private:
        std::map<std::string, z3::expr> _initial_concrete_state_valuations;
        std::map<unsigned int, std::map<std::string, z3::expr>> _cycle_to_concrete_input_valuations;
    };
}// namespace se::ahorn

#endif//AHORN_AHORN_TEST_CASE_H
