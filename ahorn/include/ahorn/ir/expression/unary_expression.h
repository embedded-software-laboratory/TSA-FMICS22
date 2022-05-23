#ifndef AHORN_UNARY_EXPRESSION_H
#define AHORN_UNARY_EXPRESSION_H

#include "expression.h"

namespace ir {
    class UnaryExpression : public Expression {
    public:
        enum class UnaryOperator {
            // Precedence: 8
            NEGATION,  // '-'
            UNARY_PLUS,// '+'
            COMPLEMENT // 'NOT' or '!'
        };

        UnaryExpression(UnaryOperator unary_operator, std::unique_ptr<Expression> operand);
        UnaryOperator getUnaryOperator() const;
        Expression &getOperand() const;
        std::unique_ptr<UnaryExpression> clone() const {
            return std::unique_ptr<UnaryExpression>(static_cast<UnaryExpression *>(this->clone_implementation()));
        }
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;

    private:
        UnaryExpression *clone_implementation() const override;

    private:
        const UnaryOperator _unary_operator;
        const std::unique_ptr<Expression> _operand;
    };
}// namespace ir

#endif//AHORN_UNARY_EXPRESSION_H
