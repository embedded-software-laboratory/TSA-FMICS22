#ifndef AHORN_SHADOW_DIVERGENCE_EVALUATOR_H
#define AHORN_SHADOW_DIVERGENCE_EVALUATOR_H

#include "ir/expression/expression_visitor.h"
#include "se/shadow/context/context.h"
#include "se/shadow/z3/solver.h"

#include "z3++.h"

#include <stack>

namespace se::shadow {
    class DivergenceEvaluator : private ir::ExpressionVisitor {
    private:
        friend class DivergenceExecutor;

    public:
        enum class ShadowProcessingMode { NONE, OLD, NEW, BOTH };

    private:
        static const std::string CONCRETE_SHADOW_VARIABLE_NAME_PREFIX;

    public:
        // XXX default constructor disabled
        DivergenceEvaluator() = delete;
        // XXX copy constructor disabled
        DivergenceEvaluator(const DivergenceEvaluator &other) = delete;
        // XXX copy assignment disabled
        DivergenceEvaluator &operator=(const DivergenceEvaluator &) = delete;

        explicit DivergenceEvaluator(Solver &solver);

        z3::expr evaluate(const ir::Expression &expression, const Context &context,
                          ShadowProcessingMode shadow_processing_mode = ShadowProcessingMode::NONE);

    private:
        void reset();

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
        bool containsShadowExpression(const DivergentState &divergent_state, const z3::expr &expression) const;

        z3::expr lowerToShadowExpression(const DivergentState &divergent_state, const z3::expr &expression,
                                         ShadowProcessingMode shadow_processing_mode) const;

    private:
        Solver *const _solver;
        unsigned int _shadow_version;
        const Context *_context;
        ShadowProcessingMode _shadow_processing_mode;
        std::stack<z3::expr> _expression_stack;
    };
}// namespace se::shadow

#endif
