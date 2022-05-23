#include "se/baseline/engine.h"
#include "se/baseline/context/context.h"

#include <stdexcept>

using namespace se::baseline;

Engine::Engine()
    : _cycle(0), _solver(std::make_unique<Solver>()), _explorer(std::make_unique<Explorer>()),
      _executor(std::make_unique<Executor>(*_solver)) {}

void Engine::run() {
    throw std::logic_error("Not implemented yet.");
}

void Engine::initialize(const Cfg &cfg) {
    _explorer->initialize(cfg);
    _executor->initialize(cfg);
    // initial state construction
    const Vertex &vertex = cfg.getEntry();
    State::store_t concrete_valuations;
    State::store_t symbolic_valuations;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(_cycle);

        const ir::DataType &data_type = it->getDataType();
        z3::expr concrete_valuation = it->hasInitialization() ? _solver->makeValue(it->getInitialization())
                                                              : _solver->makeDefaultValue(data_type);

        concrete_valuations.emplace(contextualized_name, concrete_valuation);

        // distinguish between "whole-program" input variables and local/output variables
        if (_executor->isWholeProgramInput(flattened_name)) {
            z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, data_type);
            symbolic_valuations.emplace(contextualized_name, symbolic_valuation);
        } else {
            symbolic_valuations.emplace(contextualized_name, concrete_valuation);
        }
    }
    std::vector<z3::expr> path_constraint{_solver->makeBooleanValue(true)};
    // initial context construction
    unsigned int cycle = _cycle;
    std::unique_ptr<State> state = std::make_unique<State>(vertex, std::move(concrete_valuations),
                                                           std::move(symbolic_valuations), std::move(path_constraint));
    std::deque<std::shared_ptr<Frame>> call_stack{std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel())};
    std::unique_ptr<Context> context = std::make_unique<Context>(cycle, std::move(state), std::move(call_stack));
    _explorer->pushContext(std::move(context));
}

std::vector<std::unique_ptr<Context>> Engine::step() {
    std::vector<std::unique_ptr<Context>> succeeding_cycle_contexts;
    while (!_explorer->isEmpty()) {
        std::unique_ptr<Context> context = _explorer->popContext();
        const State &state = context->getState();
        const Vertex &vertex = state.getVertex();
        unsigned int label = vertex.getLabel();
        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts =
                _executor->execute(std::move(context));
        std::pair<bool, bool> coverage = _explorer->updateCoverage(label, *succeeding_contexts.first);
        if (_cycle == succeeding_contexts.first->getCycle()) {
            _explorer->pushContext(std::move(succeeding_contexts.first));
        } else {
            assert(_cycle < succeeding_contexts.first->getCycle());
            succeeding_cycle_contexts.push_back(std::move(succeeding_contexts.first));
        }
        if (succeeding_contexts.second.has_value()) {
            std::unique_ptr<Context> forked_context = std::move(*succeeding_contexts.second);
            coverage = _explorer->updateCoverage(label, *forked_context);
            if (_cycle == forked_context->getCycle()) {
                _explorer->pushContext(std::move(forked_context));
            } else {
                assert(_cycle < forked_context->getCycle());
                succeeding_cycle_contexts.push_back(std::move(forked_context));
            }
        }
    }
    _cycle = _cycle + 1;
    return succeeding_cycle_contexts;
}