#ifndef AHORN_CONTEXT_H
#define AHORN_CONTEXT_H

#include <gtest/gtest_prod.h>

#include "se/experimental/context/frame.h"
#include "se/experimental/context/state.h"
#include "se/experimental/expression/expression.h"

#include <deque>
#include <memory>
#include <vector>

class TestLibSe_Context_Test;

namespace se {
    class Manager;

    class Context {
    private:
        FRIEND_TEST(::TestLibSe, Context);

    public:
        // XXX default constructor disabled
        Context() = delete;
        // XXX copy constructor disabled
        Context(const Context &other) = delete;
        // XXX copy assignment disabled
        Context &operator=(const Context &) = delete;

        Context(unsigned int cycle, std::unique_ptr<State> state, std::deque<std::shared_ptr<Frame>> frame_stack);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Context &context) {
            return context.print(os);
        }

        unsigned int getCycle() const;

        void setCycle(unsigned int cycle);

        State &getState();

        const State &getState() const;

        const std::deque<std::shared_ptr<Frame>> &getFrameStack() const;

        unsigned int getFrameDepth() const;

        Frame &getFrame() const;

        Frame &getMainFrame() const;

        void pushFrame(std::shared_ptr<Frame> frame);

        void popFrame();

        void pushLocalPathConstraint(std::shared_ptr<Expression> expression);

        std::string getFlattenedName(const std::string &name) const;

        // std::string getContextualizedName(const std::string &flattened_name) const;

        std::string getDecontextualizedName(const std::string &contextualized_name) const;

        std::unique_ptr<Context> fork(Manager &manager, z3::model &z3_model) const;

        std::set<std::string> extractNecessaryHardConstraints(const Manager &manager, const z3::expr &z3_expression);

        // shared ownership of expression is allowed due to argument might already be "lowered", hence we allow copy
        std::shared_ptr<Expression>
        lowerExpression(const Manager &manager, const std::shared_ptr<Expression> &expression,
                        std::map<unsigned int, std::vector<z3::expr>> &id_to_uninterpreted_constants,
                        std::map<unsigned int, std::shared_ptr<Expression>> &id_to_lowered_expression) const;

        std::shared_ptr<Expression> propagateConstantExpressions(const Manager &manager, const Expression &expression);

        // TODO (24.01.2022): Refactor this to the manager?
        bool containsWholeProgramInput(const Manager &manager, const Expression &expression) const;

    private:
        unsigned int _cycle;

        std::unique_ptr<State> _state;

        std::deque<std::shared_ptr<Frame>> _frame_stack;
    };
}// namespace se

#endif//AHORN_CONTEXT_H
