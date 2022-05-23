#include "ir/module.h"

#include <sstream>
#include <utility>

using namespace ir;

Module::Module(std::string name, Kind kind, std::unique_ptr<Interface> interface, module_ref_map_t name_to_module,
               instruction_map_t label_to_instruction, int entry_label, int exit_label)
    : Name(std::move(name)), _kind(kind), _interface(std::move(interface)), _name_to_module(std::move(name_to_module)),
      _label_to_instruction(std::move(label_to_instruction)), _entry_label(entry_label), _exit_label(exit_label) {
    for (auto it = _interface->variablesBegin(); it != _interface->variablesEnd(); ++it) {
        it->setParent(*this);
    }
    updateFlattenedInterface();
}

int Module::getEntryLabel() const {
    return _entry_label;
}

int Module::getExitLabel() const {
    return _exit_label;
}

std::ostream &Module::print(std::ostream &os) const {
    std::stringstream str;
    str << "Module " << getFullyQualifiedName() << "\n";
    for (auto &label_to_instruction : _label_to_instruction) {
        str << label_to_instruction.first << ": " << *(label_to_instruction.second) << "\n";
    }
    return os << str.str();
}

Module::instruction_map_t::const_iterator Module::instructionsBegin() const {
    return std::cbegin(_label_to_instruction);
}

Module::instruction_map_t::const_iterator Module::instructionsEnd() const {
    return std::cend(_label_to_instruction);
}

Interface &Module::getInterface() const {
    return *_interface;
}

Module::Kind Module::getKind() const {
    return _kind;
}

Module::const_variable_it Module::flattenedInterfaceBegin() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.begin(), Module::getVariableReference());
}

Module::const_variable_it Module::flattenedInterfaceEnd() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.end(), Module::getVariableReference());
}

void Module::updateFlattenedInterface() {
    _name_to_flattened_variable.clear();
    // flattens the interface of a type representative module resulting from a compiled pou, i.e., creates new
    // variables for nested variables by replacing the fully qualified name with the corresponding prefix
    for (auto variable = _interface->variablesBegin(); variable != _interface->variablesEnd(); ++variable) {
        if (variable->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // XXX Variable : DataType, e.g., f : Fb1
            std::string variable_name = variable->getName();
            // XXX Find the type representative module for the variable
            std::string module_type_name = variable->getDataType().getFullyQualifiedName();
            const Module &type_representative_module = _name_to_module.at(module_type_name);
            // XXX Replace prefix with variable name
            // XXX Fb1.x, Fb1.Fb2.y, ... --> f.x, f.g.y, ...
            for (auto it = type_representative_module.flattenedInterfaceBegin();
                 it != type_representative_module.flattenedInterfaceEnd(); ++it) {
                std::string fully_qualified_name = it->getFullyQualifiedName();
                std::string flattened_variable_name =
                        fully_qualified_name.replace(0, module_type_name.size(), variable_name);
                auto flattened_variable = std::make_unique<Variable>(std::move(flattened_variable_name),
                                                                     it->getDataType().clone(), it->getStorageType());
                flattened_variable->setParent(*this);
                _name_to_flattened_variable.emplace(flattened_variable->getName(), std::move(flattened_variable));
            }
        } else {
            std::unique_ptr<Variable> flattened_variable;
            if (variable->hasInitialization()) {
                flattened_variable =
                        std::make_unique<Variable>(variable->getName(), variable->getDataType().clone(),
                                                   variable->getStorageType(), variable->getInitialization().clone());
            } else {
                flattened_variable = std::make_unique<Variable>(variable->getName(), variable->getDataType().clone(),
                                                                variable->getStorageType());
            }
            flattened_variable->setParent(*this);
            _name_to_flattened_variable.emplace(flattened_variable->getName(), std::move(flattened_variable));
        }
    }
}
