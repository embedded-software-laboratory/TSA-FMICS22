#ifndef AHORN_ENCODER_H
#define AHORN_ENCODER_H

#include <gtest/gtest_prod.h>

#include "ir/expression/expression.h"
#include "ir/expression/expression_visitor.h"
#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"
#include "se/experimental/expression/expression.h"
#include "se/experimental/z3/manager.h"

#include <memory>
#include <stack>

class TestLibSe_Encoder_Test;

namespace se {
    class Encoder : private ir::ExpressionVisitor {
    private:
        FRIEND_TEST(::TestLibSe, Encoder);

    public:
        // XXX default constructor disabled
        Encoder() = delete;
        // XXX copy constructor disabled
        Encoder(const Encoder &other) = delete;
        // XXX copy assignment disabled
        Encoder &operator=(const Encoder &) = delete;

        Encoder(const Configuration &configuration, Manager &manager);

        std::shared_ptr<Expression> encode(const ir::Expression &expression, Context &context);

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

#endif//AHORN_ENCODER_H
