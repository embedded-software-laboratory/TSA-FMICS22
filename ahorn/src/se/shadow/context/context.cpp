#include "se/shadow/context/context.h"

#include <sstream>

using namespace se::shadow;

Context::Context(unsigned int cycle, std::unique_ptr<State> state, std::deque<std::shared_ptr<Frame>> call_stack)
    : _cycle(cycle), _state(std::move(state)), _call_stack(std::move(call_stack)) {}

std::ostream &Context::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tcycle: " << _cycle << ",\n";
    if (auto *divergent_state = dynamic_cast<const DivergentState *>(_state.get())) {
        str << *divergent_state << "\n";
    } else {
        str << "\tstate: " << *_state << "\n";
    }
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

const std::deque<std::shared_ptr<Frame>> &Context::getCallStack() const {
    return _call_stack;
}

const Frame &Context::getFrame() const {
    assert(!_call_stack.empty());
    return *_call_stack.back();
}

const Frame &Context::getMainFrame() const {
    assert(!_call_stack.empty());
    return *_call_stack.front();
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

std::unique_ptr<Context> Context::fork(Solver &solver, const Vertex &vertex, z3::model &model,
                                       const z3::expr &expression) const {
    std::unique_ptr<State> state = _state->fork(solver, vertex, model, expression);
    // XXX copy cycle and call stack
    return std::make_unique<Context>(_cycle, std::move(state), _call_stack);
}

std::unique_ptr<Context> Context::divergentFork(Solver &solver, const Vertex &vertex, z3::model &model,
                                                const z3::expr &old_encoded_expression,
                                                const z3::expr &new_encoded_expression) {
    auto *divergent_state = dynamic_cast<const DivergentState *>(&(*_state));
    assert(divergent_state != nullptr);
    std::unique_ptr<DivergentState> forked_divergent_state =
            divergent_state->divergentFork(solver, vertex, model, old_encoded_expression, new_encoded_expression);
    // XXX copy cycle and call stack
    return std::make_unique<Context>(_cycle, std::move(forked_divergent_state), _call_stack);
}