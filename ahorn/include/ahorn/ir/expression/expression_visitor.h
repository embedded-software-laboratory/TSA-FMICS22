#ifndef AHORN_IR_EXPRESSION_VISITOR_H
#define AHORN_IR_EXPRESSION_VISITOR_H

namespace ir {
    class BinaryExpression;
    class BooleanToIntegerCast;
    class ChangeExpression;
    class UnaryExpression;
    class BooleanConstant;
    class IntegerConstant;
    class TimeConstant;
    class EnumeratedValue;
    class NondeterministicConstant;
    class Undefined;
    class VariableAccess;
    class FieldAccess;
    class IntegerToBooleanCast;
    class Phi;

    class ExpressionVisitor {
    public:
        virtual ~ExpressionVisitor() = default;

        virtual void visit(const BinaryExpression &expression) = 0;
        virtual void visit(const BooleanToIntegerCast &expression) = 0;
        virtual void visit(const ChangeExpression &expression) = 0;
        virtual void visit(const UnaryExpression &expression) = 0;
        virtual void visit(const BooleanConstant &expression) = 0;
        virtual void visit(const IntegerConstant &expression) = 0;
        virtual void visit(const TimeConstant &expression) = 0;
        virtual void visit(const EnumeratedValue &expression) = 0;
        virtual void visit(const NondeterministicConstant &expression) = 0;
        virtual void visit(const Undefined &expression) = 0;
        virtual void visit(const VariableAccess &expression) = 0;
        virtual void visit(const FieldAccess &expression) = 0;
        virtual void visit(const IntegerToBooleanCast &expression) = 0;
        virtual void visit(const Phi &expression) = 0;
    };
}// namespace ir

#endif//AHORN_IR_EXPRESSION_VISITOR_H
