#include "ir/interface.h"
#include "cfg/cfg.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/module.h"
#include <sstream>

using namespace ir;

Interface::Interface(variable_vector_t variables) {
    for (auto &variable : variables) {
        std::string name = variable->getName();
        _name_to_variable.emplace(std::move(name), std::move(variable));
    }
}

Interface::const_variable_it Interface::variablesBegin() const {
    return boost::make_transform_iterator(_name_to_variable.begin(), Interface::getVariableReference());
}

Interface::const_variable_it Interface::variablesEnd() const {
    return boost::make_transform_iterator(_name_to_variable.end(), Interface::getVariableReference());
}

Interface::const_input_variable_it Interface::inputVariablesBegin() const {
    return boost::make_filter_iterator<Interface::isInputVariable>(variablesBegin(), variablesEnd());
}

Interface::const_input_variable_it Interface::inputVariablesEnd() const {
    return boost::make_filter_iterator<Interface::isInputVariable>(variablesEnd(), variablesEnd());
}

Interface::const_local_variable_it Interface::localVariablesBegin() const {
    return boost::make_filter_iterator<Interface::isLocalVariable>(variablesBegin(), variablesEnd());
}

Interface::const_local_variable_it Interface::localVariablesEnd() const {
    return boost::make_filter_iterator<Interface::isLocalVariable>(variablesEnd(), variablesEnd());
}

Interface::const_output_variable_it Interface::outputVariablesBegin() const {
    return boost::make_filter_iterator<Interface::isOutputVariable>(variablesBegin(), variablesEnd());
}

Interface::const_output_variable_it Interface::outputVariablesEnd() const {
    return boost::make_filter_iterator<Interface::isOutputVariable>(variablesEnd(), variablesEnd());
}

Interface::const_temporary_variable_it Interface::temporaryVariablesBegin() const {
    return boost::make_filter_iterator<Interface::isTemporaryVariable>(variablesBegin(), variablesEnd());
}

Interface::const_temporary_variable_it Interface::temporaryVariablesEnd() const {
    return boost::make_filter_iterator<Interface::isTemporaryVariable>(variablesEnd(), variablesEnd());
}

std::ostream &Interface::print(std::ostream &os) const {
    std::stringstream str;
    str << "Input variables: {";
    for (auto it = inputVariablesBegin(); it != inputVariablesEnd(); ++it) {
        str << *it;
        if (std::next(it) != inputVariablesEnd()) {
            str << ", ";
        }
    }
    str << "}\n";
    str << "Local variables: {";
    for (auto it = localVariablesBegin(); it != localVariablesEnd(); ++it) {
        str << *it;
        if (std::next(it) != localVariablesEnd()) {
            str << ", ";
        }
    }
    str << "}\n";
    str << "Output variables: {";
    for (auto it = outputVariablesBegin(); it != outputVariablesEnd(); ++it) {
        str << *it;
        if (std::next(it) != outputVariablesEnd()) {
            str << ", ";
        }
    }
    str << "}\n";
    str << "Temporary variables: {";
    for (auto it = temporaryVariablesBegin(); it != temporaryVariablesEnd(); ++it) {
        str << *it;
        if (std::next(it) != temporaryVariablesEnd()) {
            str << ", ";
        }
    }
    str << "}\n";
    return os << str.str();
}

Interface::variable_t Interface::getVariable(const std::string &name) const {
    if (_name_to_variable.count(name)) {
        return _name_to_variable.at(name);
    } else {
        throw std::logic_error("Variable does not exist.");
    }
}

const Variable &Interface::addVariable(variable_t variable) {
    std::string name = variable->getName();
    if (_name_to_variable.count(name) == 0) {
        auto it = _name_to_variable.emplace(std::move(name), std::move(variable));
        if (it.second) {
            return *it.first->second;
        } else {
            throw std::logic_error("Could not add variable to interface.");
        }
    } else {
        throw std::logic_error("Variable already exists.");
    }
}

void Interface::removeVariable(const std::string &name) {
    _name_to_variable.erase(name);
}