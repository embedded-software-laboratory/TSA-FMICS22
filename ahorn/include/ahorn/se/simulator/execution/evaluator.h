#ifndef AHORN_SIMULATOR_EVALUATOR_H
#define AHORN_SIMULATOR_EVALUATOR_H

#include "ir/expression/expression_visitor.h"
#include "se/simulator/context/context.h"
#include "se/shadow/z3/solver.h"

#include "z3++.h"

#include <stack>

namespace se::simulator {
    class Evaluator : private ir::ExpressionVisitor {
    public:
        enum class ShadowProcessingMode { NONE, OLD, NEW };

    public:
        // XXX default constructor disabled
        Evaluator() = delete;
        // XXX copy constructor disabled
        Evaluator(const Evaluator &other) = delete;
        // XXX copy assignment disabled
        Evaluator &operator=(const Evaluator &) = delete;

        Evaluator(ShadowProcessingMode shadow_processing_mode, shadow::Solver &solver);

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
        ShadowProcessingMode _shadow_processing_mode;
        shadow::Solver *const _solver;
        const Context *_context;
        std::stack<z3::expr> _expression_stack;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_EVALUATOR_H
