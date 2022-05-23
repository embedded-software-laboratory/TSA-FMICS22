#ifndef AHORN_BASELINE_EVALUATOR_H
#define AHORN_BASELINE_EVALUATOR_H

#include "ir/expression/expression_visitor.h"
#include "se/baseline/context/context.h"
#include "se/baseline/z3/solver.h"

#include <stack>

namespace se::baseline {
    class Evaluator : private ir::ExpressionVisitor {
    public:
        // XXX default constructor disabled
        Evaluator() = delete;
        // XXX copy constructor disabled
        Evaluator(const Evaluator &other) = delete;
        // XXX copy assignment disabled
        Evaluator &operator=(const Evaluator &) = delete;

        explicit Evaluator(Solver &solver);

        z3::expr evaluate(const ir::Expression &expression, const Context &context);

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
        Solver *const _solver;
        const Context *_context;
        std::stack<z3::expr> _expression_stack;
    };
}// namespace se::baseline

#endif//AHORN_BASELINE_EVALUATOR_H
