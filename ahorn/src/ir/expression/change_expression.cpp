#include "ir/expression/change_expression.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

ChangeExpression::ChangeExpression(std::unique_ptr<Expression> old_operand, std::unique_ptr<Expression> new_operand)
    : Expression(Kind::CHANGE), _old_operand(std::move(old_operand)), _new_operand(std::move(new_operand)) {}

void ChangeExpression::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &ChangeExpression::print(std::ostream &os) const {
    return os << "change(" << *_old_operand << ", " << *_new_operand << ")";
}

ChangeExpression *ChangeExpression::clone_implementation() const {
    std::unique_ptr<Expression> old_operand = _old_operand->clone();
    std::unique_ptr<Expression> new_operand = _new_operand->clone();
    return new ChangeExpression(std::move(old_operand), std::move(new_operand));
}

Expression &ChangeExpression::getOldOperand() const {
    return *_old_operand;
}

Expression &ChangeExpression::getNewOperand() const {
    return *_new_operand;
}

void ChangeExpression::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
