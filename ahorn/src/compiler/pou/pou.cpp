#include "compiler/pou/pou.h"

#include <sstream>

using namespace ir;

Pou::Pou(std::string name, Kind kind, std::unique_ptr<Interface> interface, pou_ref_map_t name_to_pou,
         std::unique_ptr<Body> body)
    : Name(std::move(name)), _kind(kind), _interface(std::move(interface)), _name_to_pou(std::move(name_to_pou)),
      _body(std::move(body)) {
    for (auto it = _interface->variablesBegin(); it != _interface->variablesEnd(); ++it) {
        it->setParent(*this);
    }
    updateFlattenedInterface();
}

Interface &Pou::getInterface() const {
    return *_interface;
}

const Body &Pou::getBody() const {
    return *_body;
}

Pou::Kind Pou::getKind() const {
    return _kind;
}

std::ostream &Pou::print(std::ostream &os) const {
    std::stringstream str;
    switch (_kind) {
        case Kind::PROGRAM: {
            str << "Program";
            break;
        }
        case Kind::FUNCTION_BLOCK: {
            str << "Function Block";
            break;
        }
        case Kind::FUNCTION: {
            str << "Function";
            break;
        }
    }
    str << " " << getFullyQualifiedName() << "\n";
    str << *_interface;
    str << *_body;
    return os << str.str();
}

void Pou::updateFlattenedInterface() {
    _name_to_flattened_variable.clear();
    // flattens the interface of a type representative pou, i.e., creates new
    // variables for nested variables by replacing the fully qualified name with the corresponding prefix
    for (auto variable = _interface->variablesBegin(); variable != _interface->variablesEnd(); ++variable) {
        if (variable->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // XXX Variable : DataType, e.g., f : Fb1
            std::string variable_name = variable->getName();
            // XXX Find the type representative pou for the variable
            std::string pou_type_name = variable->getDataType().getFullyQualifiedName();
            const Pou &type_representative_pou = _name_to_pou.at(pou_type_name);
            // XXX Replace prefix with variable name
            // XXX Fb1.x, Fb1.Fb2.y, ... --> f.x, f.g.y, ...
            for (auto it = type_representative_pou.flattenedInterfaceBegin();
                 it != type_representative_pou.flattenedInterfaceEnd(); ++it) {
                std::string fully_qualified_name = it->getFullyQualifiedName();
                std::string flattened_variable_name =
                        fully_qualified_name.replace(0, pou_type_name.size(), variable_name);
                auto flattened_variable = std::make_shared<Variable>(std::move(flattened_variable_name),
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

Pou::const_variable_it Pou::flattenedInterfaceBegin() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.begin(), Pou::getVariableReference());
}

Pou::const_variable_it Pou::flattenedInterfaceEnd() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.end(), Pou::getVariableReference());
}

Pou::const_pou_ref_it Pou::pousBegin() const {
    return boost::make_transform_iterator(_name_to_pou.begin(), Pou::getPouReference());
}

Pou::const_pou_ref_it Pou::pousEnd() const {
    return boost::make_transform_iterator(_name_to_pou.end(), Pou::getPouReference());
}

std::shared_ptr<Variable> Pou::getVariable(const std::string &name) const {
    auto variable = _interface->getVariable(name);
    if (variable) {
        return variable;
    }
    if (_name_to_flattened_variable.count(name)) {
        return _name_to_flattened_variable.at(name);
    } else {
        throw std::logic_error("Variable " + name + " does not exist in POU " + getFullyQualifiedName());
    }
}

std::shared_ptr<Variable> Pou::getVariable(const VariableReference &variable_reference) const {
    if (const auto *variable_access = dynamic_cast<const VariableAccess *>(&variable_reference)) {
        return variable_access->getVariable();
    }
    return std::shared_ptr<Variable>();
}
