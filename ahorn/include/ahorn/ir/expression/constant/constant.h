#ifndef AHORN_CONSTANT_H
#define AHORN_CONSTANT_H

#include "ir/expression/expression.h"

namespace ir {
    class Constant : public Expression {
    public:
        std::unique_ptr<Constant> clone() const {
            return std::unique_ptr<Constant>(this->clone_implementation());
        }

    protected:
        explicit Constant(Expression::Kind kind) : Expression(kind){};

    private:
        Constant *clone_implementation() const override = 0;
    };
}// namespace ir

#endif//AHORN_CONSTANT_H
