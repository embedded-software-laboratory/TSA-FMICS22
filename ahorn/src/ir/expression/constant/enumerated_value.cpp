#include "ir/expression/constant/enumerated_value.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"
#include <cassert>

using namespace ir;

EnumeratedValue::EnumeratedValue(std::string value)
    : Constant(Kind::ENUMERATED_VALUE_CONSTANT), _value(std::move(value)), _index(-1) {
    _type = Expression::Type::ARITHMETIC;
}

std::string EnumeratedValue::getValue() const {
    return _value;
}

void EnumeratedValue::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &EnumeratedValue::print(std::ostream &os) const {
    return os << _value;
}

EnumeratedValue *EnumeratedValue::clone_implementation() const {
    auto *enumerated_value = new EnumeratedValue(_value);
    enumerated_value->setIndex(_index);
    return enumerated_value;
}

void EnumeratedValue::setIndex(int index) {
    _index = index;
}

int EnumeratedValue::getIndex() const {
    assert(_index != -1);
    return _index;
}

void EnumeratedValue::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
