#ifndef AHORN_CBMC_CONTEXT_H
#define AHORN_CBMC_CONTEXT_H

#include "se/cbmc/context/frame.h"
#include "se/cbmc/context/state.h"

#include <deque>
#include <memory>

namespace se::cbmc {
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

        State &getState() const;

        const std::deque<std::shared_ptr<Frame>> &getCallStack() const;

        const Frame &getFrame() const;

        void pushFrame(std::shared_ptr<Frame> frame);

        void popFrame();

        unsigned int getCallStackDepth() const;

        std::unique_ptr<Context> fork(const Vertex &vertex, z3::expr next_assumption_literal,
                                      const std::string &next_assumption_literal_name, z3::expr encoded_expression);

    private:
        unsigned int _cycle;
        std::unique_ptr<State> _state;
        std::deque<std::shared_ptr<Frame>> _call_stack;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_CONTEXT_H
