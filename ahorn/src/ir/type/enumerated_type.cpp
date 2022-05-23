#include "ir/type/enumerated_type.h"

#include <cassert>
#include <ir/type/data_type.h>

using namespace ir;

EnumeratedType::EnumeratedType(std::string name)
    : DataType(std::move(name), DataType::Kind::ENUMERATED_TYPE), _index(0) {}

void EnumeratedType::addValue(std::string value) {
    _value_to_index.emplace(std::move(value), _index++);
}

EnumeratedType *EnumeratedType::clone_implementation() const {
    auto *enumerated_type = new EnumeratedType(std::move(getName()));
    for (auto &p : _value_to_index) {
        enumerated_type->addValue(p.first);
    }
    return enumerated_type;
}

std::ostream &EnumeratedType::print(std::ostream &os) const {
    return os << getName();
}

const std::map<std::string, int> &EnumeratedType::getValueToIndex() const {
    return _value_to_index;
}

int EnumeratedType::getIndex(const std::string &value) const {
    return _value_to_index.at(value);
}
