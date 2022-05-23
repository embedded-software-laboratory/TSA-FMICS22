#ifndef AHORN_BOOLEAN_TO_INTEGER_CAST_H
#define AHORN_BOOLEAN_TO_INTEGER_CAST_H

#include "ir/expression/expression.h"

namespace ir {
    class BooleanToIntegerCast : public Expression {
    public:
        BooleanToIntegerCast(std::unique_ptr<Expression> operand);
        Expression &getOperand() const;
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<BooleanToIntegerCast> clone() const {
            return std::unique_ptr<BooleanToIntegerCast>(
                    static_cast<BooleanToIntegerCast *>(this->clone_implementation()));
        }

    private:
        BooleanToIntegerCast *clone_implementation() const override;

    private:
        std::unique_ptr<Expression> _operand;
    };
}// namespace ir

#endif//AHORN_BOOLEAN_TO_INTEGER_CAST_H
