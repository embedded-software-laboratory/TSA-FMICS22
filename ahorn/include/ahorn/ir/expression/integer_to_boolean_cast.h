#ifndef AHORN_INTEGER_TO_BOOLEAN_CAST_H
#define AHORN_INTEGER_TO_BOOLEAN_CAST_H

#include "ir/expression/expression.h"

namespace ir {
    class IntegerToBooleanCast : public Expression {
    public:
        explicit IntegerToBooleanCast(std::unique_ptr<Expression> operand);
        Expression &getOperand() const;
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<IntegerToBooleanCast> clone() const {
            return std::unique_ptr<IntegerToBooleanCast>(
                    static_cast<IntegerToBooleanCast *>(this->clone_implementation()));
        }

    private:
        IntegerToBooleanCast *clone_implementation() const override;

    private:
        std::unique_ptr<Expression> _operand;
    };
}// namespace ir

#endif//AHORN_INTEGER_TO_BOOLEAN_CAST_H
