#include "ir/type/derived_type.h"

using namespace ir;

DerivedType::DerivedType(std::string name) : DataType(std::move(name), Kind::DERIVED_TYPE) {}

std::ostream &DerivedType::print(std::ostream &os) const {
    return os << getName();
}

DerivedType *DerivedType::clone_implementation() const {
    return new DerivedType(getName());
}
