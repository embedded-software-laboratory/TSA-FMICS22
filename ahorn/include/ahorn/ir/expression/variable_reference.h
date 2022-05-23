#ifndef AHORN_VARIABLE_REFERENCE_H
#define AHORN_VARIABLE_REFERENCE_H

#include "ir/expression/expression.h"

namespace ir {
    /**
     * A reference to a memory location, i.e., a variable.
     */
    class VariableReference : public Expression {
    public:
        std::unique_ptr<VariableReference> clone() const {
            return std::unique_ptr<VariableReference>(this->clone_implementation());
        }

        virtual std::string getName() const = 0;

    protected:
        explicit VariableReference(Expression::Kind type) : Expression(type){};

    private:
        VariableReference *clone_implementation() const override = 0;
    };
}// namespace ir

#endif//AHORN_VARIABLE_REFERENCE_H
