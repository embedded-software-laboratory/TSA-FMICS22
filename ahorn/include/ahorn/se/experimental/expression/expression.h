#ifndef AHORN_SE_EXPRESSION_H
#define AHORN_SE_EXPRESSION_H

#include "z3++.h"

#include <ostream>

namespace se {
    class Expression {
    public:
        enum class Kind { CONCRETE_EXPRESSION, SYMBOLIC_EXPRESSION, SHADOW_EXPRESSION, ASSUMPTION_LITERAL_EXPRESSION };

        // XXX default constructor disabled
        Expression() = delete;
        // XXX copy constructor disabled
        Expression(const Expression &other) = delete;
        // XXX copy assignment disabled
        Expression &operator=(const Expression &) = delete;

        virtual ~Expression() = default;

        virtual std::ostream &print(std::ostream &os) const = 0;

        friend std::ostream &operator<<(std::ostream &os, const Expression &expression) {
            return expression.print(os);
        }

        unsigned int getId() const;

        Kind getKind() const;

        const z3::expr &getZ3Expression() const;

    protected:
        Expression(Kind kind, z3::expr z3_expression);

        static unsigned int _next_id;

    private:
        unsigned int _id;
        Kind _kind;
        z3::expr _z3_expression;
    };
}// namespace se

#endif//AHORN_SE_EXPRESSION_H
