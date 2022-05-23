#ifndef AHORN_NON_CONST_EXPRESSION_VISITOR_H
#define AHORN_NON_CONST_EXPRESSION_VISITOR_H

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

    class NonConstExpressionVisitor {
    public:
        virtual ~NonConstExpressionVisitor() = default;

        virtual void visit(BinaryExpression &expression) = 0;
        virtual void visit(BooleanToIntegerCast &expression) = 0;
        virtual void visit(ChangeExpression &expression) = 0;
        virtual void visit(UnaryExpression &expression) = 0;
        virtual void visit(BooleanConstant &expression) = 0;
        virtual void visit(IntegerConstant &expression) = 0;
        virtual void visit(TimeConstant &expression) = 0;
        virtual void visit(EnumeratedValue &expression) = 0;
        virtual void visit(NondeterministicConstant &expression) = 0;
        virtual void visit(Undefined &expression) = 0;
        virtual void visit(VariableAccess &expression) = 0;
        virtual void visit(FieldAccess &expression) = 0;
        virtual void visit(IntegerToBooleanCast &expression) = 0;
        virtual void visit(Phi &expression) = 0;
    };
}// namespace ir

#endif//AHORN_NON_CONST_EXPRESSION_VISITOR_H
