#ifndef AHORN_CONCRETE_EXPRESSION_H
#define AHORN_CONCRETE_EXPRESSION_H

#include "se/experimental/expression/expression.h"

namespace se {
    class ConcreteExpression : public Expression {
    public:
        // XXX default constructor disabled
        ConcreteExpression() = delete;
        // XXX copy constructor disabled
        ConcreteExpression(const ConcreteExpression &other) = delete;
        // XXX copy assignment disabled
        ConcreteExpression &operator=(const ConcreteExpression &) = delete;

        explicit ConcreteExpression(z3::expr z3_expression);

        std::ostream &print(std::ostream &os) const override;
    };
}// namespace se

#endif//AHORN_CONCRETE_EXPRESSION_H
