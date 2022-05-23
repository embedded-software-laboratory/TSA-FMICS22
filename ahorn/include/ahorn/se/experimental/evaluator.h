#ifndef AHORN_EVALUATOR_H
#define AHORN_EVALUATOR_H

#include <gtest/gtest_prod.h>

#include "ir/expression/expression.h"
#include "ir/expression/expression_visitor.h"
#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"
#include "se/experimental/z3/manager.h"

class TestLibSe_Evaluator_Test;

namespace se {
    class Evaluator : private ir::ExpressionVisitor {
    private:
        FRIEND_TEST(::TestLibSe, Evaluator);

    public:
        // XXX default constructor disabled
        Evaluator() = delete;
        // XXX copy constructor disabled
        Evaluator(const Evaluator &other) = delete;
        // XXX copy assignment disabled
        Evaluator &operator=(const Evaluator &) = delete;

        Evaluator(const Configuration &configuration, Manager &manager);

        std::shared_ptr<Expression> evaluate(const ir::Expression &expression, Context &context);

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
        const Configuration &_configuration;

        unsigned int _shadow_index;

        Manager *const _manager;

        Context *_context;

        std::stack<std::shared_ptr<Expression>> _expression_stack;
    };
}// namespace se

#endif//AHORN_EVALUATOR_H
