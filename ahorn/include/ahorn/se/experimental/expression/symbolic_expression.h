#ifndef AHORN_SYMBOLIC_EXPRESSION_H
#define AHORN_SYMBOLIC_EXPRESSION_H

#include "se/experimental/expression/expression.h"

namespace se {
    class SymbolicExpression : public Expression {
    public:
        // XXX default constructor disabled
        SymbolicExpression() = delete;
        // XXX copy constructor disabled
        SymbolicExpression(const SymbolicExpression &other) = delete;
        // XXX copy assignment disabled
        SymbolicExpression &operator=(const SymbolicExpression &) = delete;

        explicit SymbolicExpression(z3::expr z3_expression);

        std::ostream &print(std::ostream &os) const override;
    };
}// namespace se

#endif//AHORN_SYMBOLIC_EXPRESSION_H
