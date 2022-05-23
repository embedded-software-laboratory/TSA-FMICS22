#include "ir/expression/integer_to_boolean_cast.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

IntegerToBooleanCast::IntegerToBooleanCast(std::unique_ptr<Expression> operand)
    : Expression(Kind::INTEGER_TO_BOOLEAN_CAST), _operand(std::move(operand)) {
    _type = Expression::Type::ARITHMETIC;
}

Expression &IntegerToBooleanCast::getOperand() const {
    return *_operand;
}

void IntegerToBooleanCast::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &IntegerToBooleanCast::print(std::ostream &os) const {
    return os << "inttobool(" << *_operand << ")";
}
IntegerToBooleanCast *IntegerToBooleanCast::clone_implementation() const {
    return new IntegerToBooleanCast(_operand->clone());
}

void IntegerToBooleanCast::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
