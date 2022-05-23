#ifndef AHORN_BINARY_EXPRESSION_H
#define AHORN_BINARY_EXPRESSION_H

#include "ir/expression/expression.h"

#include <memory>

namespace ir {
    class BinaryExpression : public Expression {
    public:
        enum class BinaryOperator {
            // Precedence: 7 ..
            EXPONENTIATION,
            // Precedence: 6 ..
            MULTIPLY,
            DIVIDE,
            MODULO,
            // Precedence: 5 ..
            ADD,
            SUBTRACT,
            // Precedence: 4 ..
            GREATER_THAN,
            LESS_THAN,
            GREATER_THAN_OR_EQUAL_TO,
            LESS_THAN_OR_EQUAL_TO,
            EQUALITY,
            INEQUALITY,
            // Precedence: 3 ..
            BOOLEAN_AND,
            // Precedence: 2 ..
            BOOLEAN_EXCLUSIVE_OR,
            // Precedence: 1 ..
            BOOLEAN_OR
        };

        BinaryExpression(BinaryExpression::BinaryOperator binary_operator, std::unique_ptr<Expression> left_operand,
                         std::unique_ptr<Expression> right_operand);
        BinaryOperator getBinaryOperator() const;
        Expression &getLeftOperand() const;
        void setLeftOperand(std::unique_ptr<Expression> expression);
        Expression &getRightOperand() const;
        void setRightOperand(std::unique_ptr<Expression> expression);
        std::unique_ptr<BinaryExpression> clone() const {
            return std::unique_ptr<BinaryExpression>(static_cast<BinaryExpression *>(this->clone_implementation()));
        }
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;

    private:
        const BinaryOperator _binary_operator;
        std::unique_ptr<Expression> _left_operand;
        std::unique_ptr<Expression> _right_operand;
        BinaryExpression *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_BINARY_EXPRESSION_H
