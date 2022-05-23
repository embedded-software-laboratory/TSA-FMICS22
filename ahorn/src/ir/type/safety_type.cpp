#include "ir/type/safety_type.h"

using namespace ir;

SafetyType *SafetyType::clone_implementation() const {
    return new SafetyType(getName());
}

SafetyType::SafetyType(const std::string &name) : DataType(name, DataType::Kind::SAFETY_TYPE) {
    if (name == "SAFEBOOL") {
        _kind = Kind::SAFEBOOL;
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

std::ostream &SafetyType::print(std::ostream &os) const {
    switch (_kind) {
        case Kind::SAFEBOOL:
            return os << "SAFEBOOL";
        default:
            throw std::logic_error("Not implemented yet.");
    }
}

SafetyType::Kind SafetyType::getType() const {
    return _kind;
}
