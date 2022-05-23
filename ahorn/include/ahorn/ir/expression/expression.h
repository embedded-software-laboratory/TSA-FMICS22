#ifndef AHORN_IR_EXPRESSION_H
#define AHORN_IR_EXPRESSION_H

#include <iostream>
#include <memory>
#include <vector>

namespace ir {
    class ExpressionVisitor;
    class NonConstExpressionVisitor;

    class Expression {
    public:
        enum class Type { UNDEFINED, ARITHMETIC, BOOLEAN };

        enum class Kind {
            BINARY_EXPRESSION,
            BOOLEAN_CONSTANT,
            INTEGER_CONSTANT,
            ENUMERATED_VALUE_CONSTANT,
            TIME_CONSTANT,
            NONDETERMINISTIC_CONSTANT,
            VARIABLE_ACCESS,
            FIELD_ACCESS,
            UNARY_EXPRESSION,
            UNDEFINED,
            PHI,
            CHANGE,
            BOOLEAN_TO_INTEGER_CAST,
            INTEGER_TO_BOOLEAN_CAST
        };

        virtual ~Expression() = default;
        virtual void accept(ExpressionVisitor &visitor) const = 0;
        virtual void accept(NonConstExpressionVisitor &visitor) = 0;
        virtual std::ostream &print(std::ostream &os) const = 0;
        friend std::ostream &operator<<(std::ostream &os, const Expression &expression) {
            return expression.print(os);
        }
        std::unique_ptr<Expression> clone() const {
            return std::unique_ptr<Expression>(this->clone_implementation());
        }

        void setType(Type type) {
            _type = type;
        };

        Type getType() const {
            return _type;
        };
        
        Kind getKind() const {
            return _kind;
        };

    protected:
        explicit Expression(Kind kind) : _kind(kind){};
        Type _type;

    private:
        Kind _kind;

    private:
        virtual Expression *clone_implementation() const = 0;
    };
}// namespace ir

#endif//AHORN_IR_EXPRESSION_H
