#ifndef AHORN_SHADOW_EXPRESSION_H
#define AHORN_SHADOW_EXPRESSION_H

#include "se/experimental/expression/expression.h"

namespace se {
    /*
     * In order to maximize sharing between the symbolic states of two program versions, whenever a change()
     * expression is encountered, a shadow expression is generated.
     * A shadow expression contains two subexpressions, one corresponding to the expression in the old program, and
     * one corresponding to the expression in the new (= changed) program.
     */
    class ShadowExpression : public Expression {
    public:
        // XXX default constructor disabled
        ShadowExpression() = delete;
        // XXX copy constructor disabled
        ShadowExpression(const ShadowExpression &other) = delete;
        // XXX copy assignment disabled
        ShadowExpression &operator=(const ShadowExpression &) = delete;

        ShadowExpression(z3::expr z3_expression, std::shared_ptr<Expression> old_expression,
                         std::shared_ptr<Expression> new_expression);

        std::ostream &print(std::ostream &os) const override;

        std::shared_ptr<Expression> getOldExpression() const;

        std::shared_ptr<Expression> getNewExpression() const;

    private:
        std::shared_ptr<Expression> _old_expression;
        std::shared_ptr<Expression> _new_expression;
    };
}// namespace se

#endif//AHORN_SHADOW_EXPRESSION_H
