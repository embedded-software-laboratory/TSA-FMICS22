#include "se/cbmc/context/context.h"

#include "spdlog/fmt/ostr.h"

#include <sstream>

using namespace se::cbmc;

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
    str << "],\n";
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

const std::deque<std::shared_ptr<Frame>> &Context::getCallStack() const {
    return _call_stack;
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

unsigned int Context::getCallStackDepth() const {
    return _call_stack.size();
}

std::unique_ptr<Context> Context::fork(const Vertex &vertex, z3::expr next_assumption_literal,
                                       const std::string &next_assumption_literal_name, z3::expr encoded_expression) {
    std::unique_ptr<State> state = _state->fork(vertex, std::move(next_assumption_literal),
                                                next_assumption_literal_name, std::move(encoded_expression));
    // XXX copy call stack
    return std::make_unique<Context>(_cycle, std::move(state), _call_stack);
}
