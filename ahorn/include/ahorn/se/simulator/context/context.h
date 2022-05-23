#ifndef AHORN_SIMULATOR_CONTEXT_H
#define AHORN_SIMULATOR_CONTEXT_H

#include "se/simulator/context/frame.h"
#include "se/simulator/context/state.h"

#include <deque>
#include <memory>

namespace se::simulator {
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

        const Frame &getFrame() const;

        void pushFrame(std::shared_ptr<Frame> frame);

        void popFrame();

    private:
        unsigned int _cycle;
        std::unique_ptr<State> _state;
        std::deque<std::shared_ptr<Frame>> _call_stack;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_CONTEXT_H
