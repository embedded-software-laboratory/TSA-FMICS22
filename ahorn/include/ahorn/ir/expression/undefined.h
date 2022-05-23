#ifndef AHORN_UNDEFINED_H
#define AHORN_UNDEFINED_H

#include "ir/expression/expression.h"

namespace ir {
    class Undefined : public Expression {
    public:
        Undefined();

        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;

        std::ostream &print(std::ostream &os) const override;

        std::unique_ptr<Undefined> clone() const {
            return std::unique_ptr<Undefined>(static_cast<Undefined *>(this->clone_implementation()));
        }

    private:
        Undefined *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_UNDEFINED_H
