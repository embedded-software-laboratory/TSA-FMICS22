#ifndef AHORN_SSA_ENCODER_H
#define AHORN_SSA_ENCODER_H

#include "ir/expression/expression.h"
#include "ir/expression/expression_visitor.h"

#include <memory>
#include <stack>

namespace pass {
    // XXX forward-declaration for dependency injection
    class SsaPass;

    class SsaEncoder : private ir::ExpressionVisitor {
    public:
        explicit SsaEncoder(SsaPass &ssa_pass);

        std::unique_ptr<ir::Expression> encode(const ir::Expression &expression);

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
        SsaPass *const _ssa_pass;
        std::stack<std::unique_ptr<ir::Expression>> _expression_stack;
    };
}// namespace pass

#endif//AHORN_SSA_ENCODER_H
