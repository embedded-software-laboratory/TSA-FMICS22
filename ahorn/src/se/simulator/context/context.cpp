#include "se/simulator/context/context.h"

#include <sstream>

using namespace se::simulator;

Context::Context(unsigned int cycle, std::unique_ptr<State> state, std::deque<std::shared_ptr<Frame>> call_stack)
    : _cycle(cycle), _state(std::move(state)), _call_stack(std::move(call_stack)) {}

std::ostream &Context::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tcycle: " << _cycle << ",\n";
    str << "\tstate: " << *_state << "\n";
    str << "\tcall stack: ";
    str << "[";
    for (auto frame = _call_stack.begin(); frame != _call_stack.end(); ++frame) {
        str << **frame;
        if (std::next(frame) != _call_stack.end()) {
            str << ", ";
        }
    }
    str << "]\n";
    str << ")";
    return os << str.str();
}

unsigned int Context::getCycle() const {
    return _cycle;
}

void Context::setCycle(unsigned int cycle) {
    _cycle = cycle;
}

State &Context::getState() const {
    return *_state;
}

const Frame &Context::getFrame() const {
    assert(!_call_stack.empty());
    return *_call_stack.back();
}

void Context::pushFrame(std::shared_ptr<Frame> frame) {
    _call_stack.push_back(std::move(frame));
}

void Context::popFrame() {
    assert(!_call_stack.empty());
    _call_stack.pop_back();
}