#include "se/smart/engine.h"

#include <stack>
#include <stdexcept>

using namespace se::smart;

Engine::Engine()
    : _cycle(0), _manager(std::make_unique<Manager>()), _explorer(std::make_unique<Explorer>()),
      _executor(std::make_unique<Executor>(*_manager)) {}

void Engine::run(const Cfg &cfg) {
    throw std::logic_error("Not implemented yet.");
}

void Engine::initialize(const Cfg &cfg) {
    std::map<std::string, unsigned int> flattened_name_to_version;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        flattened_name_to_version.emplace(std::move(flattened_name), 0);
    }
    std::set<std::string> whole_program_inputs;
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        whole_program_inputs.emplace(std::move(flattened_name));
    }
    State::store_t concrete_valuations{{0, std::map<std::string, z3::expr>({})}};
    State::store_t symbolic_valuations{{0, std::map<std::string, z3::expr>({})}};
    for (auto flattened_variable = cfg.flattenedInterfaceBegin(); flattened_variable != cfg.flattenedInterfaceEnd();
         ++flattened_variable) {
        std::string flattened_name = flattened_variable->getFullyQualifiedName();
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(_cycle);

        z3::expr concrete_valuation = flattened_variable->hasInitialization()
                                              ? _manager->makeValue(flattened_variable->getInitialization())
                                              : _manager->makeDefaultValue(flattened_variable->getDataType());

        concrete_valuations.at(0).emplace(contextualized_name, concrete_valuation);

        // distinguish between "whole-program" input variables and local/output variables
        if (whole_program_inputs.count(flattened_name)) {
            z3::expr symbolic_valuation =
                    _manager->makeConstant(contextualized_name, flattened_variable->getDataType());
            symbolic_valuations.at(0).emplace(contextualized_name, symbolic_valuation);
        } else {
            symbolic_valuations.at(0).emplace(contextualized_name, concrete_valuation);
        }
    }
    std::unique_ptr<State> state = std::make_unique<State>(
            cfg.getVertex(cfg.getEntryLabel()), std::move(concrete_valuations), std::move(symbolic_valuations));
    std::deque<std::shared_ptr<Frame>> frame_stack;
    frame_stack.push_back(std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel(), 0));
    std::unique_ptr<Context> context =
            std::make_unique<Context>(std::move(flattened_name_to_version), std::move(whole_program_inputs), _cycle,
                                      std::move(state), std::move(frame_stack));
    _explorer->pushContext(std::move(context));
}

std::vector<std::unique_ptr<Context>> Engine::step() {
    std::stack<unsigned int> stack;
    std::vector<std::unique_ptr<Context>> succeeding_cycle_contexts;
    while (!_explorer->isEmpty()) {
        std::unique_ptr<Context> context = _explorer->popContext();
        auto succeeding_context = _executor->execute(std::move(context), stack);
        if (_cycle == succeeding_context->getCycle()) {
            _explorer->pushContext(std::move(succeeding_context));
        } else {
            assert(_cycle < succeeding_context->getCycle());
            succeeding_cycle_contexts.push_back(std::move(succeeding_context));
        }
    }
    _cycle = _cycle + 1;
    return succeeding_cycle_contexts;
}