#include "se/smart/context/context.h"

#include <sstream>

using namespace se::smart;

Context::Context(std::map<std::string, unsigned int> flattened_name_to_version,
                 std::set<std::string> whole_program_inputs, unsigned int cycle, std::unique_ptr<State> state,
                 std::deque<std::shared_ptr<Frame>> frame_stack)
    : _flattened_name_to_version(std::move(flattened_name_to_version)),
      _whole_program_inputs(std::move(whole_program_inputs)), _cycle(cycle), _state(std::move(state)),
      _frame_stack(std::move(frame_stack)), _executed_conditionals(0), _stack(), _executed_conditional_to_label() {}

std::ostream &Context::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tcycle: " << _cycle << ",\n";
    str << "\tstate: " << *_state << ",\n";
    str << "\tframe stack: ";
    str << "[";
    for (auto frame = _frame_stack.begin(); frame != _frame_stack.end(); ++frame) {
        str << **frame;
        if (std::next(frame) != _frame_stack.end()) {
            str << ", ";
        }
    }
    str << "],\n";
    str << "\texecuted conditionals: " << _executed_conditionals << "\n";
    str << ")";
    return os << str.str();
}

unsigned int Context::getVersion(const std::string &flattened_name) const {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    return _flattened_name_to_version.at(flattened_name);
}

void Context::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

bool Context::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
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
    assert(!_frame_stack.empty());
    return *_frame_stack.back();
}

void Context::pushFrame(std::shared_ptr<Frame> frame) {
    _frame_stack.push_back(std::move(frame));
}

void Context::popFrame() {
    assert(!_frame_stack.empty());
    _frame_stack.pop_back();
}

int Context::getExecutedConditionals() const {
    return _executed_conditionals;
}

void Context::setExecutedConditionals(int executed_conditionals, unsigned int label) {
    _executed_conditionals = executed_conditionals;
    _executed_conditional_to_label.emplace(_executed_conditionals, label);
}

void Context::resetExecutedConditionals() {
    _executed_conditionals = 0;
    _executed_conditional_to_label.clear();
}

std::vector<unsigned int> &Context::getStack() {
    return _stack;
}

void Context::backtrack(int j, z3::model &model, std::vector<z3::expr> path_constraint) {
    const Frame &frame = *_frame_stack.back();
    const Cfg &cfg = frame.getCfg();
    unsigned int k = j + 1;
    const Vertex &vertex = cfg.getVertex(_executed_conditional_to_label.at(k));
    _executed_conditionals = j;
    _state->backtrack(vertex, j, model, std::move(path_constraint));
}