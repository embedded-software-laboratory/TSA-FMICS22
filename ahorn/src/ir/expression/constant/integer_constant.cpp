#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

IntegerConstant::IntegerConstant(int value) : Constant(Expression::Kind::INTEGER_CONSTANT), _value(value) {
    _type = Expression::Type::ARITHMETIC;
}

std::ostream &IntegerConstant::print(std::ostream &os) const {
    return os << _value;
}

IntegerConstant *IntegerConstant::clone_implementation() const {
    return new IntegerConstant(_value);
}

void IntegerConstant::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

int IntegerConstant::getValue() const {
    return _value;
}

void IntegerConstant::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
