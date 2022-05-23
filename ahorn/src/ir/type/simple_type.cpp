#include "ir/type/simple_type.h"

using namespace ir;

SimpleType::SimpleType(std::string name) : DataType(std::move(name), DataType::Kind::SIMPLE_TYPE) {}

SimpleType *SimpleType::clone_implementation() const {
    return new SimpleType(getName());
}

std::ostream &SimpleType::print(std::ostream &os) const {
    return os << "SimpleType";
}