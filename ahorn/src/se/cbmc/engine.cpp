#include "se/cbmc/engine.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::cbmc;

Engine::Engine(std::unique_ptr<Configuration> configuration)
    : _configuration(std::move(configuration)), _cycle(0), _solver(std::make_unique<Solver>()),
      _explorer(std::make_unique<Explorer>()), _executor(std::make_unique<Executor>(*_solver, *_explorer)),
      _merger(std::make_unique<Merger>(*_solver)) {
    _termination_criteria = TerminationCriteria::CYCLE_BOUND;
    if (_configuration->_cycle_bound.has_value()) {
        _cycle_bound = *(_configuration->_cycle_bound);
    }
}

void Engine::run(const Cfg &cfg) {
    using namespace std::literals;
    auto logger = spdlog::get("CBMC");
    initialize(cfg);
    _begin_time_point = std::chrono::system_clock::now();
    while (!isTerminationCriteriaMet()) {
        SPDLOG_LOGGER_INFO(logger, "#############################################");
        SPDLOG_LOGGER_INFO(logger, "Cycle: {}", _cycle);
        SPDLOG_LOGGER_INFO(logger, "Explorer:\n{}", *_explorer);
        SPDLOG_LOGGER_INFO(logger, "#############################################");
        std::unique_ptr<Context> succeeding_cycle_context = step();
        assert(succeeding_cycle_context != nullptr);
        _explorer->push(std::move(succeeding_cycle_context));
    }
    auto elapsed_time = (std::chrono::system_clock::now() - _begin_time_point) / 1ms;
    std::cout << "Run took " << elapsed_time << "ms." << std::endl;
    std::cout << *_explorer << std::endl;
}

bool Engine::isTerminationCriteriaMet() const {
    auto logger = spdlog::get("CBMC");
    bool termination_criteria_met = true;
    if (_termination_criteria & TerminationCriteria::CYCLE_BOUND) {
        if (_cycle < _cycle_bound) {
            termination_criteria_met = false;
        } else {
            SPDLOG_LOGGER_TRACE(logger, "Termination criteria \"CYCLE_BOUND\" met, terminating engine.");
            return true;
        }
    }
    return termination_criteria_met;
}

void Engine::initialize(const Cfg &cfg) {
    _solver->initialize(cfg);
    _explorer->initialize(cfg);
    _executor->initialize(cfg);
    _merger->initialize(cfg);
    // initial state construction
    const Vertex &vertex = cfg.getEntry();
    std::string assumption_literal_name =
            "b_" + cfg.getName() + "_" + std::to_string(cfg.getEntryLabel()) + "__" + std::to_string(_cycle);
    z3::expr assumption_literal = _solver->makeBooleanConstant(assumption_literal_name);
    std::map<std::string, std::vector<z3::expr>> assumption_literals;
    assumption_literals.emplace(assumption_literal_name, std::vector<z3::expr>{_solver->makeBooleanValue(true)});
    std::map<std::string, std::vector<z3::expr>> assumptions;
    std::map<std::string, std::map<std::string, z3::expr>> hard_constraints;
    std::map<std::string, z3::expr> initial_valuations;
    std::map<std::string, unsigned int> flattened_name_to_version;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        flattened_name_to_version.emplace(flattened_name, 0);

        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(_cycle);

        const ir::DataType &data_type = it->getDataType();
        z3::expr concrete_valuation = it->hasInitialization() ? _solver->makeValue(it->getInitialization())
                                                              : _solver->makeDefaultValue(data_type);

        // distinguish between "whole-program" input variables and local/output variables
        if (_executor->isWholeProgramInput(flattened_name)) {
            z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, data_type);
            initial_valuations.emplace(contextualized_name, symbolic_valuation);
        } else {
            initial_valuations.emplace(contextualized_name, concrete_valuation);
        }
    }
    std::unique_ptr<State> state = std::make_unique<State>(
            std::move(initial_valuations), std::move(flattened_name_to_version), vertex, assumption_literal,
            std::move(assumption_literals), std::move(assumptions), std::move(hard_constraints));
    // initial context construction
    unsigned int cycle = _cycle;
    std::deque<std::shared_ptr<Frame>> call_stack{std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel())};
    std::unique_ptr<Context> context = std::make_unique<Context>(cycle, std::move(state), std::move(call_stack));
    _explorer->push(std::move(context));
}

std::unique_ptr<Context> Engine::step() {
    auto logger = spdlog::get("CBMC");
    std::unique_ptr<Context> succeeding_cycle_context;
    while (!_explorer->isEmpty() || !_merger->isEmpty()) {
        SPDLOG_LOGGER_INFO(logger, "explorer: \n{}", *_explorer);
        SPDLOG_LOGGER_INFO(logger, "merger: \n{}", *_merger);
        if (_explorer->isEmpty()) {
            SPDLOG_LOGGER_TRACE(logger, "No more contexts for exploration exist, falling back on merge.");
            assert(!_merger->isEmpty());
            std::unique_ptr<Context> merged_context = _merger->merge();
            _explorer->push(std::move(merged_context));
        } else {
            std::unique_ptr<Context> context = _explorer->pop();
            std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts =
                    _executor->execute(std::move(context));
            std::unique_ptr<Context> succeeding_context = std::move(succeeding_contexts.first);
            _explorer->checkGoals(*_solver, *succeeding_context);
            // succeeding context
            if (_merger->reachedMergePoint(*succeeding_context)) {
                SPDLOG_LOGGER_TRACE(logger, "Reached merge point.");
                _merger->pushContext(std::move(succeeding_context));
            } else {
                if (_cycle == succeeding_context->getCycle()) {
                    _explorer->push(std::move(succeeding_context));
                } else {
                    assert(_cycle < succeeding_context->getCycle());
                    assert(!succeeding_contexts.second.has_value());
                    assert(!_merger->reachedMergePoint(*succeeding_context));
                    succeeding_cycle_context = std::move(succeeding_context);
                }
            }
            // forked succeeding context
            if (succeeding_contexts.second.has_value()) {
                SPDLOG_LOGGER_INFO(logger, "Forked");
                std::unique_ptr<Context> forked_succeeding_context = std::move(*succeeding_contexts.second);
                SPDLOG_LOGGER_INFO(logger, "Forked context:\n{}", *forked_succeeding_context);
                _explorer->checkGoals(*_solver, *forked_succeeding_context);
                if (_merger->reachedMergePoint(*forked_succeeding_context)) {
                    SPDLOG_LOGGER_TRACE(logger, "Reached merge point.");
                    _merger->pushContext(std::move(forked_succeeding_context));
                } else {
                    assert(_cycle == forked_succeeding_context->getCycle());
                    _explorer->push(std::move(forked_succeeding_context));
                }
            }
        }
    }
    _cycle = _cycle + 1;
    return succeeding_cycle_context;
}