#ifndef AHORN_OA_CONTEXT_H
#define AHORN_OA_CONTEXT_H

#include "se/oa/context/frame.h"
#include "se/oa/context/state.h"
#include "se/oa/z3/solver.h"

#include <deque>
#include <memory>

namespace se::oa {
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

    private:
        unsigned int _cycle;
        std::unique_ptr<State> _state;
        std::deque<std::shared_ptr<Frame>> _call_stack;
    };
}// namespace se::oa

#endif//AHORN_OA_CONTEXT_H