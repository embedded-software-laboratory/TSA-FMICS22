#include "se/simulator/engine.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <chrono>

using namespace se::simulator;

Engine::Engine(Evaluator::ShadowProcessingMode shadow_processing_mode, shadow::Solver &solver)
    : _cycle(0), _solver(&solver), _explorer(std::make_unique<Explorer>()),
      _executor(std::make_unique<Executor>(shadow_processing_mode, *_solver)) {}

std::pair<std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                       std::vector<unsigned int>>>>,
          std::map<unsigned int, std::map<std::string, z3::expr>>>
Engine::run(const Cfg &cfg, const std::map<std::string, z3::expr> &initial_concrete_state_valuations,
            const std::map<unsigned int, std::map<std::string, z3::expr>> &cycle_to_concrete_input_valuations) {
    using namespace std::literals;
    std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();
    auto logger = spdlog::get("Simulator");
    // XXX state = local + output valuations
    assert(!initial_concrete_state_valuations.empty());
    initialize(cfg);
    std::unique_ptr<Context> context = initializeFromConcreteValuation(cfg, initial_concrete_state_valuations);
    assert(!cycle_to_concrete_input_valuations.empty());
    State &state = context->getState();
    std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                 std::vector<unsigned int>>>>
            execution_history;
    std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_state_valuations;
    for (const auto &cycle_to_valuations : cycle_to_concrete_input_valuations) {
        unsigned int cycle = cycle_to_valuations.first;
        // XXX update concrete input valuations
        for (const auto &concrete_input_valuation : cycle_to_valuations.second) {
            std::string flattened_name = concrete_input_valuation.first;
            std::string contextualized_name = flattened_name + "__" + std::to_string(cycle);
            state.updateConcreteValuation(contextualized_name, concrete_input_valuation.second);
        }
        step(*context, execution_history);
        // retrieve simulated local and output valuations
        cycle_to_concrete_state_valuations.emplace(cycle, std::map<std::string, z3::expr>());
        for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
            std::string flattened_name = it->getFullyQualifiedName();
            std::string contextualized_name = flattened_name + "__" + std::to_string(cycle);
            if (!_executor->isWholeProgramInput(flattened_name)) {
                cycle_to_concrete_state_valuations.at(cycle).emplace(flattened_name,
                                                                     state.getConcreteValuation(contextualized_name));
            }
        }
    }
    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
    SPDLOG_LOGGER_INFO(logger, "Simulation took {}ms, simulated {} cycle(s).", elapsed_time, _cycle);
    SPDLOG_LOGGER_TRACE(logger, "Simulation reached statement and branch coverage: ({}, {})",
                       _explorer->_statement_coverage, _explorer->_branch_coverage);
    SPDLOG_LOGGER_TRACE(logger, "Simulated execution history:\n{}.", execution_history);
    SPDLOG_LOGGER_TRACE(logger, "Simulated state valuations reaching end of respective cycle:\n{}.",
                        cycle_to_concrete_state_valuations);
    return std::make_pair(execution_history, cycle_to_concrete_state_valuations);
}

void Engine::run(const Cfg &cfg, const std::vector<std::shared_ptr<se::shadow::TestCase>> &test_cases) {
    using namespace std::literals;
    std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();
    auto logger = spdlog::get("Simulator");
    initialize(cfg);
    for (const auto &test_case : test_cases) {
        std::unique_ptr<Context> context =
                initializeFromConcreteValuation(cfg, test_case->getInitialConcreteStateValuations());
        State &state = context->getState();
        for (const auto &cycle_to_concrete_input_valuations : test_case->getCycleToConcreteInputValuations()) {
            unsigned int cycle = cycle_to_concrete_input_valuations.first;
            // XXX update concrete input valuations
            for (const auto &concrete_input_valuation : cycle_to_concrete_input_valuations.second) {
                std::string flattened_name = concrete_input_valuation.first;
                std::string contextualized_name = flattened_name + "__" + std::to_string(cycle);
                state.updateConcreteValuation(contextualized_name, concrete_input_valuation.second);
            }
            step(*context);
        }
        reset();
    }
    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
    SPDLOG_LOGGER_INFO(logger, "Simulation took {}ms.", elapsed_time);
    SPDLOG_LOGGER_INFO(logger, "Simulation reached statement and branch coverage: ({}, {})",
                        _explorer->_statement_coverage, _explorer->_branch_coverage);
}

void Engine::initialize(const Cfg &cfg) {
    _cycle = 0;
    _explorer->initialize(cfg);
    _executor->initialize(cfg);
}

std::unique_ptr<Context>
Engine::initializeFromConcreteValuation(const Cfg &cfg,
                                        const std::map<std::string, z3::expr> &initial_concrete_state_valuations) {
    // initial state construction
    const Vertex &vertex = cfg.getEntry();
    std::map<std::string, z3::expr> concrete_valuations;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        std::string contextualized_name = flattened_name + "__" + std::to_string(_cycle);
        // distinguish between "whole-program" input variables and local/output variables
        if (_executor->isWholeProgramInput(flattened_name)) {
            // XXX input valuations get overwritten during execution (driven by the valuations of the test case)
            const ir::DataType &data_type = it->getDataType();
            z3::expr concrete_valuation = it->hasInitialization() ? _solver->makeValue(it->getInitialization())
                                                                  : _solver->makeDefaultValue(data_type);
            concrete_valuations.emplace(contextualized_name, concrete_valuation);
        } else {
            z3::expr concrete_valuation = initial_concrete_state_valuations.at(flattened_name);
            concrete_valuations.emplace(contextualized_name, concrete_valuation);
        }
    }
    std::unique_ptr<State> state = std::make_unique<State>(vertex, std::move(concrete_valuations));
    // initial context construction
    unsigned int cycle = _cycle;
    std::deque<std::shared_ptr<Frame>> call_stack{std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel())};
    std::unique_ptr<Context> context = std::make_unique<Context>(cycle, std::move(state), std::move(call_stack));
    return context;
}

void Engine::reset() {
    _cycle = 0;
}

void Engine::step(Context &context) {
    while (context.getCycle() == _cycle) {
        unsigned int label = context.getState().getVertex().getLabel();
        const Frame &frame = context.getFrame();
        const std::string &scope = frame.getScope();
        const Cfg &cfg = frame.getCfg();
        _executor->execute(context);
        _explorer->updateCoverage(label, context);
    }
    _cycle = _cycle + 1;
}

void Engine::step(
        Context &context,
        std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                     std::vector<unsigned int>>>> &execution_history) {
    while (context.getCycle() == _cycle) {
        unsigned int cycle = context.getCycle();
        unsigned int label = context.getState().getVertex().getLabel();
        const Frame &frame = context.getFrame();
        const std::string &scope = frame.getScope();
        const Cfg &cfg = frame.getCfg();
        if (execution_history.find(cycle) == execution_history.end()) {
            execution_history.emplace(
                    cycle,
                    std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                          std::vector<unsigned int>>>({std::pair(
                            std::pair(scope, std::reference_wrapper(cfg)), std::vector<unsigned int>({label}))}));
        } else {
            const auto &local_execution_history = execution_history.at(cycle).back();
            if (local_execution_history.first.first == scope) {
                execution_history.at(cycle).back().second.push_back(label);
            } else {
                execution_history.at(cycle).push_back(
                        std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>, std::vector<unsigned int>>(
                                std::pair(std::pair(scope, std::reference_wrapper(cfg)),
                                          std::vector<unsigned int>({label}))));
            }
        }
        _executor->execute(context);
        _explorer->updateCoverage(label, context);
    }
    _cycle = _cycle + 1;
}