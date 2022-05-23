#ifndef AHORN_CHANGE_EXPRESSION_H
#define AHORN_CHANGE_EXPRESSION_H

#include "ir/expression/expression.h"

namespace ir {
    class ChangeExpression : public Expression {
    public:
        ChangeExpression(std::unique_ptr<Expression> old_operand, std::unique_ptr<Expression> new_operand);

        Expression &getOldOperand() const;

        Expression &getNewOperand() const;

        std::unique_ptr<ChangeExpression> clone() const {
            return std::unique_ptr<ChangeExpression>(static_cast<ChangeExpression *>(this->clone_implementation()));
        }

        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;

        std::ostream &print(std::ostream &os) const override;

    private:
        ChangeExpression *clone_implementation() const override;

    private:
        std::unique_ptr<Expression> _old_operand;
        std::unique_ptr<Expression> _new_operand;
    };
}// namespace ir

#endif//AHORN_CHANGE_EXPRESSION_H
