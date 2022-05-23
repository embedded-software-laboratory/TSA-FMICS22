#include "ir/expression/boolean_to_integer_cast.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

BooleanToIntegerCast::BooleanToIntegerCast(std::unique_ptr<Expression> operand)
    : Expression(Kind::BOOLEAN_TO_INTEGER_CAST), _operand(std::move(operand)) {
    _type = Expression::Type::BOOLEAN;
}

Expression &BooleanToIntegerCast::getOperand() const {
    return *_operand;
}

void BooleanToIntegerCast::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &BooleanToIntegerCast::print(std::ostream &os) const {
    return os << "booltoint(" << *_operand << ")";
}

BooleanToIntegerCast *BooleanToIntegerCast::clone_implementation() const {
    return new BooleanToIntegerCast(_operand->clone());
}

void BooleanToIntegerCast::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
