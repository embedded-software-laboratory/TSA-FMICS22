#include "ir/expression/constant/time_constant.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

TimeConstant::TimeConstant(int value) : Constant(Kind::TIME_CONSTANT), _value(value) {
    _type = Expression::Type::ARITHMETIC;
}

std::ostream &TimeConstant::print(std::ostream &os) const {
    return os << _value;
}

TimeConstant *TimeConstant::clone_implementation() const {
    return new TimeConstant(_value);
}

void TimeConstant::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

int TimeConstant::getValue() const {
    return _value;
}

void TimeConstant::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
