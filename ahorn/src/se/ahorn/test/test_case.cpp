#include "se/ahorn/test/test_case.h"

using namespace se::ahorn;

TestCase::TestCase(std::map<std::string, z3::expr> initial_concrete_state_valuations,
                   std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_input_valuations)
    : _initial_concrete_state_valuations(std::move(initial_concrete_state_valuations)),
      _cycle_to_concrete_input_valuations(std::move(cycle_to_concrete_input_valuations)) {}

std::ostream &TestCase::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tinitial concrete state valuations: [";
    for (auto it = _initial_concrete_state_valuations.begin(); it != _initial_concrete_state_valuations.end();
         ++it) {
        str << it->first << " -> " << it->second;
        if (std::next(it) != _initial_concrete_state_valuations.end()) {
            str << ", ";
        }
    }
    str << "],\n";
    str << "\tconcrete input valuations: {\n";
    for (auto it = _cycle_to_concrete_input_valuations.begin(); it != _cycle_to_concrete_input_valuations.end(); ++it) {
        str << "\t\tcycle " << it->first << ": [";
        for (auto valuation = it->second.begin(); valuation != it->second.end(); ++valuation) {
            str << valuation->first << " -> " << valuation->second;
            if (std::next(valuation) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]";
        if (std::next(it) != _cycle_to_concrete_input_valuations.end()) {
            str << ",\n";
        }
    }
    str << "\n\t}";
    str << "\n)";
    return os << str.str();
}

const std::map<std::string, z3::expr> &TestCase::getInitialConcreteStateValuations() const {
    return _initial_concrete_state_valuations;
}

const std::map<unsigned int, std::map<std::string, z3::expr>> &TestCase::getCycleToConcreteInputValuations() const {
    return _cycle_to_concrete_input_valuations;
}