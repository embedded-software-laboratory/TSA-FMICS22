#include "se/shadow/test/test_case.h"

#include <sstream>

using namespace se::shadow;

TestCase::TestCase(std::map<std::string, z3::expr> initial_concrete_state_valuations,
                   std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_input_valuations)
    : _initial_concrete_state_valuations(std::move(initial_concrete_state_valuations)),
      _cycle_to_concrete_input_valuations(std::move(cycle_to_concrete_input_valuations)),
      _execution_history(boost::none), _cycle_to_concrete_state_valuations(boost::none) {}

std::ostream &TestCase::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    if (_execution_history.has_value()) {
        str << "\texecution history: {\n";
        for (auto it1 = (*_execution_history).begin(); it1 != (*_execution_history).end(); ++it1) {
            str << "\t\tcycle " << it1->first << ": [\n";
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
                str << "\t\t\t(\"" << it2->first.first << "\", " << (it2->first.second.get().getName()) << "): [";
                for (auto it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                    str << (*it3);
                    if (std::next(it3) != it2->second.end()) {
                        str << ", ";
                    }
                }
                str << "]";
                if (std::next(it2) != it1->second.end()) {
                    str << ",\n";
                }
            }
            str << "\n\t\t]";
            if (std::next(it1) != (*_execution_history).end()) {
                str << ",\n";
            }
        }
        str << "\n\t},\n";
    }
    str << "\tinitial concrete state valuations: [";
    for (auto it = _initial_concrete_state_valuations.begin(); it != _initial_concrete_state_valuations.end(); ++it) {
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
    if (_cycle_to_concrete_state_valuations.has_value()) {
        str << ",\n";
        str << "\tconcrete state valuations (expected reaching cycle end): {\n";
        for (auto it = (*_cycle_to_concrete_state_valuations).begin();
             it != (*_cycle_to_concrete_state_valuations).end(); ++it) {
            str << "\t\tcycle " << it->first << ": [";
            for (auto valuation = it->second.begin(); valuation != it->second.end(); ++valuation) {
                str << valuation->first << " -> " << valuation->second;
                if (std::next(valuation) != it->second.end()) {
                    str << ", ";
                }
            }
            str << "]";
            if (std::next(it) != (*_cycle_to_concrete_state_valuations).end()) {
                str << ",\n";
            }
        }
        str << "\n\t}";
    }
    str << "\n)";
    return os << str.str();
}

const std::map<std::string, z3::expr> &TestCase::getInitialConcreteStateValuations() const {
    return _initial_concrete_state_valuations;
}

void TestCase::setInitialConcreteStateValuation(const std::string &flattened_name, const z3::expr &valuation) {
    assert(_initial_concrete_state_valuations.find(flattened_name) == _initial_concrete_state_valuations.end());
    _initial_concrete_state_valuations.emplace(flattened_name, valuation);
}

const std::map<unsigned int, std::map<std::string, z3::expr>> &TestCase::getCycleToConcreteInputValuations() const {
    return _cycle_to_concrete_input_valuations;
}

void TestCase::setCycleToConcreteInputValuation(unsigned int cycle, const std::string &flattened_name,
                                                const z3::expr &valuation) {
    assert(_cycle_to_concrete_input_valuations.find(cycle) != _cycle_to_concrete_input_valuations.end());
    assert(_cycle_to_concrete_input_valuations.at(cycle).find(flattened_name) ==
           _cycle_to_concrete_input_valuations.at(cycle).end());
    _cycle_to_concrete_input_valuations.at(cycle).emplace(flattened_name, valuation);
}

const std::map<
        unsigned int,
        std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>, std::vector<unsigned int>>>> &
TestCase::getExecutionHistory() const {
    assert(_execution_history.has_value());
    return *_execution_history;
}

void TestCase::setExecutionHistory(
        std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                     std::vector<unsigned int>>>>
                execution_history) {
    _execution_history = std::move(execution_history);
}

const std::map<unsigned int, std::map<std::string, z3::expr>> &TestCase::getCycleToConcreteStateValuations() const {
    assert(_cycle_to_concrete_state_valuations.has_value());
    return *_cycle_to_concrete_state_valuations;
}

void TestCase::setCycleToConcreteStateValuations(
        std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_state_valuations) {
    _cycle_to_concrete_state_valuations = std::move(cycle_to_concrete_state_valuations);
}