#ifndef AHORN_SHADOW_CONTEXT_H
#define AHORN_SHADOW_CONTEXT_H

#include "se/shadow/context/divergent_state.h"
#include "se/shadow/context/frame.h"
#include "se/shadow/context/state.h"
#include "se/shadow/z3/solver.h"

#include <deque>
#include <memory>

namespace se::shadow {
    class Context {
    public:
        // XXX default constructor disabled
        Context() = delete;
        // XXX copy constructor disabled
        Context(const Context &other) = delete;
        // XXX copy assignment disabled
        Context &operator=(const Context &) = delete;

        Context(unsigned int cycle, std::unique_ptr<State> state, std::deque<std::shared_ptr<Frame>> call_stack);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Context &context) {
            return context.print(os);
        }

        unsigned int getCycle() const;

        void setCycle(unsigned int cycle);

        // XXX modifiable
        State &getState() const;

        const std::deque<std::shared_ptr<Frame>> &getCallStack() const;

        const Frame &getFrame() const;

        const Frame &getMainFrame() const;

        void pushFrame(std::shared_ptr<Frame> frame);

        void popFrame();

        unsigned int getCallStackDepth() const;

        std::unique_ptr<Context> fork(Solver &solver, const Vertex &vertex, z3::model &model,
                                      const z3::expr &expression) const;

        std::unique_ptr<Context> divergentFork(Solver &solver, const Vertex &vertex, z3::model &model,
                                               const z3::expr &old_encoded_expression,
                                               const z3::expr &new_encoded_expression);

    private:
        unsigned int _cycle;
        std::unique_ptr<State> _state;
        std::deque<std::shared_ptr<Frame>> _call_stack;
    };
}// namespace se::shadow

#endif