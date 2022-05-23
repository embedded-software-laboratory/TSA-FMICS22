#include "ir/expression/binary_expression.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

#include <algorithm>
#include <sstream>

using namespace ir;

BinaryExpression::BinaryExpression(BinaryExpression::BinaryOperator binary_operator,
                                   std::unique_ptr<Expression> left_operand, std::unique_ptr<Expression> right_operand)
    : Expression(Kind::BINARY_EXPRESSION), _binary_operator(binary_operator), _left_operand(std::move(left_operand)),
      _right_operand(std::move(right_operand)) {
    switch (_binary_operator) {
        case BinaryOperator::EXPONENTIATION:
        case BinaryOperator::MULTIPLY:
        case BinaryOperator::DIVIDE:
        case BinaryOperator::MODULO:
        case BinaryOperator::ADD:
        case BinaryOperator::SUBTRACT: {
            _type = Expression::Type::ARITHMETIC;
            break;
        }
        case BinaryOperator::GREATER_THAN:
        case BinaryOperator::LESS_THAN:
        case BinaryOperator::GREATER_THAN_OR_EQUAL_TO:
        case BinaryOperator::LESS_THAN_OR_EQUAL_TO:
        case BinaryOperator::EQUALITY:
        case BinaryOperator::INEQUALITY:
        case BinaryOperator::BOOLEAN_AND:
        case BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        case BinaryOperator::BOOLEAN_OR: {
            _type = Expression::Type::BOOLEAN;
            break;
        }
        default:
            throw std::logic_error("Invalid binary operator.");
    }
}

std::ostream &BinaryExpression::print(std::ostream &os) const {
    std::stringstream str;
    switch (_binary_operator) {
        case BinaryOperator::EXPONENTIATION:
            str << "**";
            break;
        case BinaryOperator::MULTIPLY:
            str << "*";
            break;
        case BinaryOperator::DIVIDE:
            str << "/";
            break;
        case BinaryOperator::MODULO:
            str << "MOD";
            break;
        case BinaryOperator::ADD:
            str << "+";
            break;
        case BinaryOperator::SUBTRACT:
            str << "-";
            break;
        case BinaryOperator::GREATER_THAN:
            str << ">";
            break;
        case BinaryOperator::LESS_THAN:
            str << "<";
            break;
        case BinaryOperator::GREATER_THAN_OR_EQUAL_TO:
            str << ">=";
            break;
        case BinaryOperator::LESS_THAN_OR_EQUAL_TO:
            str << "<=";
            break;
        case BinaryOperator::EQUALITY:
            str << "=";
            break;
        case BinaryOperator::INEQUALITY:
            str << "<>";
            break;
        case BinaryOperator::BOOLEAN_AND:
            str << "AND";
            break;
        case BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
            str << "XOR";
            break;
        case BinaryOperator::BOOLEAN_OR:
            str << "OR";
            break;
    }
    return os << *_left_operand << " " << str.str() << " " << *_right_operand;
}

BinaryExpression *BinaryExpression::clone_implementation() const {
    std::unique_ptr<Expression> left_operand = _left_operand->clone();
    std::unique_ptr<Expression> right_operand = _right_operand->clone();
    return new BinaryExpression(_binary_operator, std::move(left_operand), std::move(right_operand));
}

void BinaryExpression::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

Expression &BinaryExpression::getLeftOperand() const {
    return *_left_operand;
}

Expression &BinaryExpression::getRightOperand() const {
    return *_right_operand;
}

BinaryExpression::BinaryOperator BinaryExpression::getBinaryOperator() const {
    return _binary_operator;
}

void BinaryExpression::setRightOperand(std::unique_ptr<Expression> expression) {
    _right_operand = std::move(expression);
}

void BinaryExpression::setLeftOperand(std::unique_ptr<Expression> expression) {
    _left_operand = std::move(expression);
}

void BinaryExpression::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
