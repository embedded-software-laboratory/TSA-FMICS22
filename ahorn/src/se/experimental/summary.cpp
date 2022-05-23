#include "se/experimental/summary.h"

#include <sstream>

using namespace se;

Summary::Summary(
        std::shared_ptr<AssumptionLiteralExpression> entry_assumption_literal,
        std::shared_ptr<AssumptionLiteralExpression> exit_assumption_literal,
        std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator>
                assumption_literals,
        std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                assumptions,
        std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints)
    : _entry_assumption_literal(std::move(entry_assumption_literal)),
      _exit_assumption_literal(std::move(exit_assumption_literal)),
      _assumption_literals(std::move(assumption_literals)), _assumptions(std::move(assumptions)),
      _hard_constraints(std::move(hard_constraints)) {}

std::ostream &Summary::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tentry assumption literal: " << *_entry_assumption_literal << ",\n";
    str << "\texit assumption literal: " << *_exit_assumption_literal << ",\n";
    str << "\tassumption literals: {\n";
    for (auto it = _assumption_literals.begin(); it != _assumption_literals.end(); ++it) {
        str << "\t\t"
            << "\"" << it->first << "\""
            << " -> " << *it->second;
        if (std::next(it) != _assumption_literals.end()) {
            str << ",\n";
        }
    }
    str << "\n\t},\n";
    str << "\tassumptions: {";
    if (_assumptions.empty()) {
        str << "},\n";
    } else {
        str << "\n";
        for (auto it = _assumptions.begin(); it != _assumptions.end(); ++it) {
            str << "\t\t"
                << "\"" << it->first << "\""
                << ": [";
            for (auto assumption_literal_it = it->second.begin(); assumption_literal_it != it->second.end();
                 ++assumption_literal_it) {
                str << *(*assumption_literal_it);
                if (std::next(assumption_literal_it) != it->second.end()) {
                    str << ", ";
                }
            }
            str << "]";
            if (std::next(it) != _assumptions.end()) {
                str << ",\n";
            }
        }
        str << "\n\t},\n";
    }
    str << "\thard constraints: {";
    if (_hard_constraints.empty()) {
        str << "}\n";
    } else {
        str << "\n";
        for (auto it = _hard_constraints.begin(); it != _hard_constraints.end(); ++it) {
            str << "\t\t"
                << "\"" << it->first << "\""
                << ": [";
            for (auto valuation = it->second.begin(); valuation != it->second.end(); ++valuation) {
                str << valuation->first << " = " << valuation->second;
                if (std::next(valuation) != it->second.end()) {
                    str << ", ";
                }
            }
            str << "]";
            if (std::next(it) != _hard_constraints.end()) {
                str << ",\n";
            }
        }
        str << "\n\t}\n";
    }
    str << ")";
    return os << str.str();
}

const std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator> &
Summary::getAssumptionLiterals() const {
    return _assumption_literals;
}

const std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator> &
Summary::getAssumptions() const {
    return _assumptions;
}

const std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> &
Summary::getHardConstraints() const {
    return _hard_constraints;
}