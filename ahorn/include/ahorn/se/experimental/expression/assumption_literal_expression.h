#ifndef AHORN_ASSUMPTION_LITERAL_EXPRESSION_H
#define AHORN_ASSUMPTION_LITERAL_EXPRESSION_H

#include "se/experimental/expression/expression.h"

#include <ostream>

namespace se {
    class AssumptionLiteralExpression : public Expression {
    public:
        // XXX default constructor disabled
        AssumptionLiteralExpression() = delete;
        // XXX copy constructor disabled
        AssumptionLiteralExpression(const AssumptionLiteralExpression &other) = delete;
        // XXX copy assignment disabled
        AssumptionLiteralExpression &operator=(const AssumptionLiteralExpression &) = delete;

        explicit AssumptionLiteralExpression(z3::expr z3_expression);

        std::ostream &print(std::ostream &os) const override;
    };
}

#endif//AHORN_ASSUMPTION_LITERAL_EXPRESSION_H
