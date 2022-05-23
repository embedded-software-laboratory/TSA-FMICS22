#include "ir/type/inconclusive_type.h"

using namespace ir;

InconclusiveType::InconclusiveType(std::string name) : DataType(std::move(name), DataType::Kind::INCONCLUSIVE_TYPE) {}

InconclusiveType *InconclusiveType::clone_implementation() const {
    return new InconclusiveType(getName());
}

std::ostream &InconclusiveType::print(std::ostream &os) const {
    return os << "InconclusiveType";
}
