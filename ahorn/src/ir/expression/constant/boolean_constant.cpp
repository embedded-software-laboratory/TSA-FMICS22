#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

BooleanConstant::BooleanConstant(bool value) : Constant(Kind::BOOLEAN_CONSTANT), _value(value) {
    _type = Expression::Type::BOOLEAN;
}

std::ostream &BooleanConstant::print(std::ostream &os) const {
    return os << (_value ? "TRUE" : "FALSE");
}

BooleanConstant *BooleanConstant::clone_implementation() const {
    return new BooleanConstant(_value);
}

void BooleanConstant::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

bool BooleanConstant::getValue() const {
    return _value;
}

void BooleanConstant::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
