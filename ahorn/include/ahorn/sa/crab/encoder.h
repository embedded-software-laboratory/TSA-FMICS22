#ifndef AHORN_SA_ENCODER_H
#define AHORN_SA_ENCODER_H

#include "ir/expression/expression.h"
#include "ir/expression/expression_visitor.h"

namespace sa {
    // XXX forward-declaration for dependency injection
    class Marshaller;
    
    class Encoder : private ir::ExpressionVisitor {
    public:
        explicit Encoder(Marshaller &marshaller);

        void encode(const ir::Expression &expression);

    private:
        void visit(const ir::BinaryExpression &expression) override;
        void visit(const ir::BooleanToIntegerCast &expression) override;
        void visit(const ir::ChangeExpression &expression) override;
        void visit(const ir::UnaryExpression &expression) override;
        void visit(const ir::BooleanConstant &expression) override;
        void visit(const ir::IntegerConstant &expression) override;
        void visit(const ir::TimeConstant &expression) override;
        void visit(const ir::EnumeratedValue &expression) override;
        void visit(const ir::NondeterministicConstant &expression) override;
        void visit(const ir::Undefined &expression) override;
        void visit(const ir::VariableAccess &expression) override;
        void visit(const ir::FieldAccess &expression) override;
        void visit(const ir::IntegerToBooleanCast &expression) override;
        void visit(const ir::Phi &expression) override;

    private:
        Marshaller *const _marshaller;
    };
}// namespace sa

#endif//AHORN_SA_ENCODER_H
