#include "ir/expression/unary_expression.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

#include <sstream>

using namespace ir;

UnaryExpression::UnaryExpression(UnaryExpression::UnaryOperator unary_operator, std::unique_ptr<Expression> operand)
    : Expression(Kind::UNARY_EXPRESSION), _unary_operator(unary_operator), _operand(std::move(operand)) {
    switch (_unary_operator) {
        case UnaryOperator::NEGATION:
        case UnaryOperator::UNARY_PLUS: {
            _type = Expression::Type::ARITHMETIC;
            break;
        }
        case UnaryOperator::COMPLEMENT: {
            _type = Expression::Type::BOOLEAN;
            break;
        }
        default:
            throw std::logic_error("Invalid unary operator.");
    }
}

void UnaryExpression::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &UnaryExpression::print(std::ostream &os) const {
    std::stringstream str;
    switch (_unary_operator) {
        case UnaryOperator::NEGATION:
            str << "-";
            break;
        case UnaryOperator::UNARY_PLUS:
            str << "+";
            break;
        case UnaryOperator::COMPLEMENT:
            str << "NOT";
            break;
    }
    return os << str.str() << "(" << *_operand << ")";
}

UnaryExpression *UnaryExpression::clone_implementation() const {
    std::unique_ptr<Expression> operand = _operand->clone();
    return new UnaryExpression(_unary_operator, std::move(operand));
}

Expression &UnaryExpression::getOperand() const {
    return *_operand;
}

UnaryExpression::UnaryOperator UnaryExpression::getUnaryOperator() const {
    return _unary_operator;
}

void UnaryExpression::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
