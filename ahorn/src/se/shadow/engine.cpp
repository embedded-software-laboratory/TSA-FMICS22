#include "se/shadow/engine.h"
#include "pass/change_annotation_collection_pass.h"
#include "se/simulator/engine.h"

#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <chrono>

using namespace se::shadow;

Engine::Engine(std::unique_ptr<Configuration> configuration)
    : _configuration(std::move(configuration)), _cycle(0), _solver(std::make_unique<Solver>()),
      _explorer(std::make_unique<Explorer>()), _divergence_executor(std::make_unique<DivergenceExecutor>(*_solver)),
      _executor(std::make_unique<Executor>(*_solver)), _merger(std::make_unique<Merger>(*_solver, *_executor)),
      _test_suite(std::make_unique<TestSuite>(*_solver)) {
    _termination_criteria = TerminationCriteria::CYCLE_BOUND | TerminationCriteria::COVERAGE;
    if (_configuration->_cycle_bound.has_value()) {
        _cycle_bound = *(_configuration->_cycle_bound);
    }
}

void Engine::run(const Cfg &cfg, const std::string &path) {
    auto logger = spdlog::get("Shadow");
    using namespace std::literals;
    std::chrono::time_point<std::chrono::system_clock> begin_collect_change_traversing_test_cases =
            std::chrono::system_clock::now();
    std::vector<std::shared_ptr<TestCase>> change_traversing_test_cases = collectChangeTraversingTestCases(cfg, path);
    auto elapsed_time_collect_change_traversing_test_cases =
            (std::chrono::system_clock::now() - begin_collect_change_traversing_test_cases) / 1ms;

    if (change_traversing_test_cases.empty()) {
        SPDLOG_LOGGER_TRACE(logger, "Either no change annotated label or no test case touching any change annotated "
                                    "label exists, stopping engine.");
        return;
    }

    // XXX symbolic execution phase: for each test case "touching" the patch, execute shadow symbolic execution
    // Phase 1
    std::chrono::time_point<std::chrono::system_clock> begin_phase_1 = std::chrono::system_clock::now();
    std::vector<std::pair<std::unique_ptr<Context>, std::shared_ptr<TestCase>>> Q_divergent;
    for (const std::shared_ptr<TestCase> &test_case : change_traversing_test_cases) {
        std::vector<std::pair<std::unique_ptr<Context>, std::shared_ptr<TestCase>>> divergent_contexts_and_test_cases =
                findDivergentContexts(cfg, test_case);
        if (divergent_contexts_and_test_cases.empty()) {
            SPDLOG_LOGGER_TRACE(logger, "No divergent contexts were derived for test case touching change.");
        } else {
            for (auto &divergent_context_and_test_case : divergent_contexts_and_test_cases) {
                Q_divergent.push_back(std::move(divergent_context_and_test_case));
            }
        }
    }
    long elapsed_time_phase_1 = (std::chrono::system_clock::now() - begin_phase_1) / 1ms;

    // Phase 2
    std::chrono::time_point<std::chrono::system_clock> begin_phase_2 = std::chrono::system_clock::now();
    std::vector<std::shared_ptr<TestCase>> divergent_test_cases = std::vector<std::shared_ptr<TestCase>>();
    float progress = 0.0;
    int bar_width = 100;
    for (auto &divergent_context_and_test_case : Q_divergent) {
        std::vector<std::shared_ptr<TestCase>> test_cases = performBoundedExecution(
                cfg, std::move(divergent_context_and_test_case.first), *divergent_context_and_test_case.second);
        for (std::shared_ptr<TestCase> &test_case : test_cases) {
            // TODO: This check is redundant as performBoundedExecution(...) already checks in _test_suite->deriveTest(
            //  ...) for the existence of test cases with equal valuations
            // XXX check if there already exists a test case with the same initial concrete state valuation and concrete
            // XXX input valuation.
            auto it = std::find_if(divergent_test_cases.begin(), divergent_test_cases.end(),
                                   [&test_case](const std::shared_ptr<TestCase> &existing_test_case) {
                                       return *test_case == *existing_test_case;
                                   });
            if (it == divergent_test_cases.end()) {
                divergent_test_cases.push_back(std::move(test_case));
            } else {
                SPDLOG_LOGGER_INFO(logger, "A test case with the same valuation has already been added, skipping "
                                           "augmentation of divergent test cases.");
            }
        }
        std::stringstream progress_bar;
        int position = int(progress * bar_width);
        for (int i = 0; i <= bar_width; ++i) {
            if (i <= position) {
                progress_bar << "|";
            }
        }
        progress += ((float) 1) / (float) Q_divergent.size();
        progress_bar << " " << std::roundf(progress * 100) << "%";
        SPDLOG_LOGGER_INFO(logger, "{}", progress_bar.str());
    }
    long elapsed_time_phase_2 = (std::chrono::system_clock::now() - begin_phase_2) / 1ms;

    std::chrono::time_point<std::chrono::system_clock> begin_check_for_output_differences =
            std::chrono::system_clock::now();
    std::vector<std::pair<std::shared_ptr<TestCase>, std::shared_ptr<TestCase>>> difference_revealing_test_cases =
            checkForOutputDifferences(cfg, divergent_test_cases);
    auto elapsed_time_check_for_output_differences =
            (std::chrono::system_clock::now() - begin_check_for_output_differences) / 1ms;
    SPDLOG_LOGGER_TRACE(logger, "difference revealing test cases:");
    for (const auto &test_cases : difference_revealing_test_cases) {
        SPDLOG_LOGGER_TRACE(logger, "old:\n{}", *test_cases.first);
        SPDLOG_LOGGER_TRACE(logger, "new:\n{}", *test_cases.second);
    }

    // XXX check coverage of divergent test cases on new version of the program
    SPDLOG_LOGGER_INFO(logger, "Checking coverage of divergent test cases on new version of the program...");
    auto simulator = std::make_unique<simulator::Engine>(simulator::Evaluator::ShadowProcessingMode::NEW, *_solver);
    simulator->run(cfg, divergent_test_cases);

    std::stringstream statistics;
    statistics << "#change annotated labels: " << _change_annotated_labels << ",\n";
    statistics << "#untouched change annotated labels: " << _untouched_change_annotated_labels << ",\n";
    statistics << "#change-traversing test cases: " << _change_traversing_test_cases << ",\n";
    // statistics << "#considered change-traversing test cases: " << change_traversing_test_cases.size() << ",\n";
    statistics << "Phase 1: #found divergent contexts: " << Q_divergent.size() << ",\n";
    statistics << "Phase 2: #derived divergent test cases: " << divergent_test_cases.size() << ",\n";
    statistics << "Coverage of derived divergent test cases in new version: ("
               << simulator->_explorer->_statement_coverage << ", " << simulator->_explorer->_branch_coverage << ")"
               << std::endl;
    statistics << "#difference-revealing test cases: " << difference_revealing_test_cases.size();
    std::cout << "Statistics: \n" << statistics.str() << std::endl;
    std::cout << "Collecting change traversing test cases took " << elapsed_time_collect_change_traversing_test_cases
              << "ms." << std::endl;
    std::cout << "Phase 1: Symbolic execution took " << elapsed_time_phase_1 << "ms." << std::endl;
    std::cout << "Phase 2: Bounded symbolic execution took " << elapsed_time_phase_2 << "ms." << std::endl;
    std::cout << "Phase 1 + Phase 2 took " << elapsed_time_phase_1 + elapsed_time_phase_2 << "ms in total."
              << std::endl;
    std::cout << "Checking for output differences took " << elapsed_time_check_for_output_differences << "ms."
              << std::endl;
    /*
    if (_configuration->_generate_test_suite.has_value()) {
        _test_suite->toXML(*(_configuration->_generate_test_suite));
    }
    */
}

// C.f. Kuchta et al gathering each input that touches the patch
std::vector<std::shared_ptr<TestCase>> Engine::collectChangeTraversingTestCases(const Cfg &cfg,
                                                                                const std::string &path) {
    auto logger = spdlog::get("Shadow");

    auto change_annotation_collection_pass = std::make_unique<pass::ChangeAnnotationCollectionPass>();
    std::set<unsigned int> change_annotated_labels = change_annotation_collection_pass->apply(cfg);

    std::vector<std::shared_ptr<TestCase>> test_cases;
    if (boost::filesystem::is_directory(path)) {
        for (const auto &directory_iterator : boost::filesystem::directory_iterator(path)) {
            const std::string &file_path = directory_iterator.path().string();
            if (file_path.find(".xml") != std::string::npos) {
                test_cases.push_back(fromXML(file_path));
            }
        }
    } else {
        test_cases.push_back(fromXML(path));
    }

    // XXX augment test case with additional
    const ir::Interface &interface = cfg.getInterface();
    std::set<std::string> whole_program_inputs;
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        whole_program_inputs.emplace(std::move(flattened_name));
    }
    for (auto &test_case : test_cases) {
        SPDLOG_LOGGER_TRACE(logger, *test_case);
        const std::map<std::string, z3::expr> &initial_concrete_state_valuations =
                test_case->getInitialConcreteStateValuations();
        const std::map<unsigned int, std::map<std::string, z3::expr>> &cycle_to_concrete_input_valuations =
                test_case->getCycleToConcreteInputValuations();
        for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
            std::string flattened_name = it->getFullyQualifiedName();
            if (whole_program_inputs.find(flattened_name) == whole_program_inputs.end()) {
                if (initial_concrete_state_valuations.find(flattened_name) == initial_concrete_state_valuations.end()) {
                    SPDLOG_LOGGER_TRACE(logger, "Augmenting test case with added state variable \"{}\"...",
                                        flattened_name);
                    const ir::DataType &data_type = it->getDataType();
                    test_case->setInitialConcreteStateValuation(flattened_name, _solver->makeDefaultValue(data_type));
                }
            } else {
                for (unsigned int cycle = 0; cycle <= cycle_to_concrete_input_valuations.rbegin()->first; ++cycle) {
                    if (cycle_to_concrete_input_valuations.at(cycle).find(flattened_name) ==
                        cycle_to_concrete_input_valuations.at(cycle).end()) {
                        SPDLOG_LOGGER_TRACE(logger,
                                            "Augmenting test case with added input variable \"{}\" for cycle "
                                            "{}...",
                                            flattened_name, cycle);
                        const ir::DataType &data_type = it->getDataType();
                        test_case->setCycleToConcreteInputValuation(cycle, flattened_name,
                                                                    _solver->makeDefaultValue(data_type));
                    }
                }
            }
        }
    }

    // XXX determine which test cases "touch" the change, a test case can "touch" multiple change annotated labels
    std::set<unsigned int> untouched_change_annotated_labels = change_annotated_labels;
    std::vector<std::shared_ptr<TestCase>> change_traversing_test_cases;
    std::map<std::size_t, std::set<unsigned int>> test_case_to_change_annotated_labels;
    for (std::size_t i = 0; i < test_cases.size(); ++i) {
        std::shared_ptr<TestCase> &test_case = test_cases.at(i);
        auto simulator = std::make_unique<simulator::Engine>(simulator::Evaluator::ShadowProcessingMode::OLD, *_solver);
        auto result = simulator->run(cfg, test_case->getInitialConcreteStateValuations(),
                                     test_case->getCycleToConcreteInputValuations());
        std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                     std::vector<unsigned int>>>>
                execution_history = result.first;
        std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_output_valuations = result.second;
        std::set<unsigned int> flattened_execution_history;
        std::for_each(execution_history.begin(), execution_history.end(),
                      [&flattened_execution_history](const auto &p) {
                          for (auto it = p.second.begin(); it != p.second.end(); ++it) {
                              flattened_execution_history.insert(it->second.begin(), it->second.end());
                          }
                      });
        // collect all change-annotated labels this test case is touching
        std::set<unsigned int> touched_change_annotated_labels;
        bool test_case_augmented = false;
        for (unsigned int label : flattened_execution_history) {
            if (change_annotated_labels.count(label)) {
                touched_change_annotated_labels.insert(label);
                // XXX check which change-annotated labels are covered by this test case
                untouched_change_annotated_labels.erase(label);
                if (!test_case_augmented) {
                    // XXX augment test case with additional information such as execution history and state valuations
                    // XXX reaching cycle end of the old program version
                    test_case->setExecutionHistory(execution_history);
                    test_case->setCycleToConcreteStateValuations(cycle_to_concrete_output_valuations);
                    change_traversing_test_cases.push_back(test_case);
                    // XXX only add the test case once
                    test_case_augmented = true;
                }
            }
        }
        test_case_to_change_annotated_labels.emplace(i, touched_change_annotated_labels);
    }

    // for each change-annotated label, choose one representative test case
    /*
    std::set<unsigned int> considered_test_case;
    for (unsigned int label : change_annotated_labels) {
        std::size_t i = 0;
        std::size_t size = 0;
        for (const auto &p : test_case_to_change_annotated_labels) {
            if (p.second.count(label)) {
                if (p.second.size() > size) {
                    i = p.first;
                    size = p.second.size();
                }
            }
        }
        if (size > 0) {
            considered_test_case.insert(i);
        }
    }
    */

    if (!untouched_change_annotated_labels.empty()) {
        std::stringstream str;
        SPDLOG_LOGGER_INFO(logger, "#Untouched change annotated labels: {}", untouched_change_annotated_labels.size());
        for (unsigned int untouched_change_annotated_label : untouched_change_annotated_labels) {
            str << untouched_change_annotated_label << ", ";
        }
        SPDLOG_LOGGER_INFO(logger, "labels: {}", str.str());
        // throw std::logic_error("Not implemented yet.");
    }

    _change_annotated_labels = change_annotated_labels.size();
    _change_traversing_test_cases = change_traversing_test_cases.size();
    _untouched_change_annotated_labels = untouched_change_annotated_labels.size();

    /*
    std::vector<std::shared_ptr<TestCase>> considered_change_traversing_test_cases;
    considered_change_traversing_test_cases.reserve(considered_test_case.size());
    for (unsigned int i : considered_test_case) {
        considered_change_traversing_test_cases.push_back(change_traversing_test_cases.at(i));
    }
    */

    // return considered_change_traversing_test_cases;
    return change_traversing_test_cases;
}

// XXX Shadow symbolic execution operates in two phases: concolic phase and bounded symbolic execution phase
// C.f. Kuchta et al. Concolic Phase aka Phase 1
std::vector<std::pair<std::unique_ptr<Context>, std::shared_ptr<TestCase>>>
Engine::findDivergentContexts(const Cfg &cfg, std::shared_ptr<TestCase> test_case) {
    auto logger = spdlog::get("Shadow");
    SPDLOG_LOGGER_TRACE(logger, "\n{}", *test_case);
    reset();
    std::unique_ptr<Context> context = initializeDivergenceExecution(cfg, *test_case);
    State &state = context->getState();
    // XXX Collect test cases that trigger divergences; these are the test cases used in the final simulation to
    // XXX check whether the both versions have differing outputs.
    std::vector<std::pair<std::unique_ptr<Context>, std::shared_ptr<TestCase>>> divergent_contexts_and_test_cases;
    SPDLOG_LOGGER_TRACE(logger, "Trying to find divergent contexts using test case:\n{}", *test_case);
    for (const auto &cycle_to_concrete_input_valuations : test_case->getCycleToConcreteInputValuations()) {
        unsigned int cycle = cycle_to_concrete_input_valuations.first;
        for (const auto &concrete_input_valuation : cycle_to_concrete_input_valuations.second) {
            std::string flattened_name = concrete_input_valuation.first;
            std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(cycle);
            state.updateConcreteValuation(contextualized_name, concrete_input_valuation.second);
        }
    continue_finding_divergent_contexts:
        auto result = stepUntilDivergence(*context);
        DivergenceExecutor::ExecutionStatus execution_status = result.first;
        if (execution_status == DivergenceExecutor::ExecutionStatus::DIVERGENT_BEHAVIOR) {
            // XXX concrete executions diverge, i.e., the test case makes both program versions follow different
            // XXX sides at a branch containing a change shadow. However, this test case might not be sufficient to
            // XXX explore all new behaviors, hence add it to the queue to enable a bounded symbolic execution in the
            // XXX new version
            assert(result.second.empty());
            SPDLOG_LOGGER_TRACE(logger, "Stopping execution as input exposes a divergence for context:\n{}", *context);
            SPDLOG_LOGGER_TRACE(logger, "Adding context to queue for the second phase.");
            divergent_contexts_and_test_cases.emplace_back(std::move(context), std::move(test_case));
            break;
        } else if (execution_status == DivergenceExecutor::ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR) {
            SPDLOG_LOGGER_TRACE(logger, "Potential divergent behavior encountered.");
            if (result.second.empty()) {
                SPDLOG_LOGGER_TRACE(logger, "No divergences were feasible and hence no divergent context was created "
                                            "during concolic execution.");
            } else {
                // XXX at least one "diff"-path of the four-way fork is feasible and the resulting divergent contexts
                // XXX must be explored. First, derive a test case that exercises the divergent behavior, and then
                // XXX continue with the bounded symbolic execution in the new version in order to systematically and
                // XXX comprehensively explore additional divergent contexts. As long as the concrete executions do
                // XXX not diverge (i.e., ExecutionStatus::DIVERGENT_BEHAVIOR), trying to find divergent contexts is
                // XXX performed until the end of the test case exploring any additional possible divergences along the
                // XXX way.
                for (std::unique_ptr<Context> &divergent_context : result.second) {
                    SPDLOG_LOGGER_TRACE(logger,
                                        "Execution of test case resulted in four-way fork of divergent "
                                        "context:\n{}",
                                        *divergent_context);
                    SPDLOG_LOGGER_TRACE(logger, "Deriving test case for divergent context.");
                    std::shared_ptr<TestCase> derived_test_case = _test_suite->deriveTestCase(*divergent_context);
                    if (derived_test_case) {
                        auto simulator = std::make_unique<simulator::Engine>(
                                simulator::Evaluator::ShadowProcessingMode::OLD, *_solver);
                        auto simulation_result =
                                simulator->run(cfg, derived_test_case->getInitialConcreteStateValuations(),
                                               derived_test_case->getCycleToConcreteInputValuations());
                        std::map<unsigned int,
                                 std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                       std::vector<unsigned int>>>>
                                execution_history = simulation_result.first;
                        std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_output_valuations =
                                simulation_result.second;
                        // XXX augment test case with additional information such as execution history and state valuations
                        // XXX reaching cycle end of the old program version
                        derived_test_case->setExecutionHistory(execution_history);
                        derived_test_case->setCycleToConcreteStateValuations(cycle_to_concrete_output_valuations);
                        divergent_contexts_and_test_cases.emplace_back(std::move(divergent_context), derived_test_case);
                    } else {
                        SPDLOG_LOGGER_TRACE(logger, "Skipping simulation as derived test case already exists in test "
                                                    "suite.");
                    }
                }
            }
            // XXX continue concolic execution with the context triggering the potential divergent behavior
            goto continue_finding_divergent_contexts;
        } else {
            // XXX continue concolic execution
            assert(execution_status == DivergenceExecutor::ExecutionStatus::EXPECTED_BEHAVIOR);
        }
    }
    return divergent_contexts_and_test_cases;
}

// C.f. Kuchta et al bounded symbolic execution (BSE) phase aka Phase 2
std::vector<std::shared_ptr<TestCase>>
Engine::performBoundedExecution(const Cfg &cfg, std::unique_ptr<Context> divergent_context, const TestCase &test_case) {
    using namespace std::literals;
    auto logger = spdlog::get("Shadow");
    std::vector<std::shared_ptr<TestCase>> test_cases = std::vector<std::shared_ptr<TestCase>>();
    // XXX Reset of engine before processing each divergent context to guarantee consistent state
    reset();
    // XXX the divergent contexts and test cases hold the contexts that triggered either a diverging concrete
    // XXX execution (i.e. ExecutionStatus::DIVERGENT_BEHAVIOR) or were generated because of potential, possible
    // XXX divergences at the four-way fork. The corresponding test case is responsible for triggering the
    // XXX divergent context.
    initializeBoundedExecution(cfg, *divergent_context, test_case);
    SPDLOG_LOGGER_TRACE(logger, "Starting bounded symbolic execution from divergent context:\n{}", *divergent_context);
    std::unique_ptr<Context> context = translateDivergentContext(std::move(divergent_context));
    SPDLOG_LOGGER_TRACE(logger, "Resulting translated divergent context:\n{}", *context);
    _explorer->push(std::move(context));
    _begin_time_point = std::chrono::system_clock::now();
    // XXX Start a BSE for each divergent context
    while (!isTerminationCriteriaMet()) {
        std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();
        std::pair<std::unique_ptr<Context>, boost::optional<TerminationCriteria>> result = stepUntilBound(test_cases);
        auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
        if (result.second.has_value()) {
            SPDLOG_LOGGER_INFO(logger,
                               "Step took {}ms, terminating bounded execution as local termination "
                               "criteria is met.",
                               elapsed_time);
        } else {
            std::unique_ptr<Context> succeeding_cycle_context = std::move(result.first);
            SPDLOG_LOGGER_INFO(logger, "Step took {}ms, current statement and branch coverage: ({}, {})", elapsed_time,
                               _explorer->_statement_coverage, _explorer->_branch_coverage);
            SPDLOG_LOGGER_TRACE(logger, "Resulting succeeding cycle context:\n{}", *succeeding_cycle_context);
            _explorer->push(std::move(succeeding_cycle_context));
        }
    }
    auto elapsed_time = (std::chrono::system_clock::now() - _begin_time_point) / 1ms;
    SPDLOG_LOGGER_INFO(logger, "Run took {}ms.", elapsed_time);
    SPDLOG_LOGGER_INFO(logger, "Engine reached the following statement and branch coverage: ({}, {})",
                       _explorer->_statement_coverage, _explorer->_branch_coverage);
    SPDLOG_LOGGER_INFO(logger, "A total of {} branch-covering test cases were created.",
                       _test_suite->_test_cases.size());
    return test_cases;
}

std::vector<std::pair<std::shared_ptr<TestCase>, std::shared_ptr<TestCase>>>
Engine::checkForOutputDifferences(const Cfg &cfg, const std::vector<std::shared_ptr<TestCase>> &divergent_test_cases) {
    auto logger = spdlog::get("Shadow");
    SPDLOG_LOGGER_TRACE(logger, "Checking old and new version for output differences on test cases which exposed "
                                "divergent contexts...");
    for (const std::shared_ptr<TestCase> &test_case : divergent_test_cases) {
        SPDLOG_LOGGER_TRACE(logger, "\n{}", *test_case);
    }
    // XXX In order to determine whether a test case exposes an externally observable difference, the outputs on the
    // XXX test case in the new version are compared to the outputs on the test case in the old version.
    std::vector<std::pair<std::shared_ptr<TestCase>, std::shared_ptr<TestCase>>> difference_revealing_test_cases;
    for (const std::shared_ptr<TestCase> &test_case : divergent_test_cases) {
        auto simulator = std::make_unique<simulator::Engine>(simulator::Evaluator::ShadowProcessingMode::NEW, *_solver);
        auto result = simulator->run(cfg, test_case->getInitialConcreteStateValuations(),
                                     test_case->getCycleToConcreteInputValuations());
        std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                     std::vector<unsigned int>>>>
                execution_history = result.first;
        std::map<unsigned int, std::map<std::string, z3::expr>> new_cycle_to_concrete_output_valuations = result.second;
        std::map<unsigned int, std::map<std::string, z3::expr>> old_cycle_to_concrete_output_valuations =
                test_case->getCycleToConcreteStateValuations();
        assert(new_cycle_to_concrete_output_valuations.size() == old_cycle_to_concrete_output_valuations.size());
        bool found_difference_revealing_test_case = false;
        for (const auto &cycle_to_concrete_output_valuations : old_cycle_to_concrete_output_valuations) {
            if (found_difference_revealing_test_case) {
                break;
            }
            for (const auto &concrete_output_valuation : cycle_to_concrete_output_valuations.second) {
                if (!z3::eq(concrete_output_valuation.second,
                            new_cycle_to_concrete_output_valuations.at(cycle_to_concrete_output_valuations.first)
                                    .at(concrete_output_valuation.first))) {
                    SPDLOG_LOGGER_TRACE(logger, "Outputs in old and new version differ, adding difference revealing "
                                                "test case...");
                    std::shared_ptr<TestCase> new_test_case =
                            std::make_shared<TestCase>(test_case->getInitialConcreteStateValuations(),
                                                       test_case->getCycleToConcreteInputValuations());
                    new_test_case->setExecutionHistory(execution_history);
                    new_test_case->setCycleToConcreteStateValuations(new_cycle_to_concrete_output_valuations);
                    difference_revealing_test_cases.emplace_back(test_case, new_test_case);
                    found_difference_revealing_test_case = true;
                    break;
                }
            }
        }
    }
    return difference_revealing_test_cases;
}

std::unique_ptr<Context> Engine::translateDivergentContext(std::unique_ptr<Context> divergent_context) {
    const Frame &frame = divergent_context->getFrame();
    const Cfg &cfg = frame.getCfg();
    State &state = divergent_context->getState();
    auto *divergent_state = dynamic_cast<DivergentState *>(&state);
    assert(divergent_state != nullptr);
    // translated state construction
    const Vertex &vertex = divergent_state->getVertex();
    std::map<std::string, z3::expr> concrete_valuations;
    const std::map<std::string, std::pair<z3::expr, z3::expr>> &concrete_shadow_valuations =
            divergent_state->getConcreteShadowValuations();
    for (const auto &concrete_valuation : divergent_state->getConcreteValuations()) {
        if (containsShadowExpression(divergent_state->getConcreteValuations(), concrete_shadow_valuations,
                                     concrete_valuation.second)) {
            z3::expr lowered_expression = lowerToShadowExpression(
                    divergent_state->getConcreteValuations(), concrete_shadow_valuations, concrete_valuation.second);
            // XXX use new valuation for exploration of divergent context
            concrete_valuations.emplace(concrete_valuation.first, lowered_expression);
        } else {
            concrete_valuations.emplace(concrete_valuation);
        }
    }
    std::map<std::string, z3::expr> symbolic_valuations;
    const std::map<std::string, std::pair<z3::expr, z3::expr>> &symbolic_shadow_valuations =
            divergent_state->getSymbolicShadowValuations();
    for (const auto &symbolic_valuation : divergent_state->getSymbolicValuations()) {
        if (containsShadowExpression(divergent_state->getSymbolicValuations(), symbolic_shadow_valuations,
                                     symbolic_valuation.second)) {
            z3::expr lowered_expression = lowerToShadowExpression(
                    divergent_state->getSymbolicValuations(), symbolic_shadow_valuations, symbolic_valuation.second);
            // XXX use new valuation for exploration of divergent context
            symbolic_valuations.emplace(symbolic_valuation.first, lowered_expression);
        } else {
            symbolic_valuations.emplace(symbolic_valuation);
        }
    }
    // XXX continue bounded symbolic execution in the new version, i.e., consider only the common and the "new" path
    // XXX constraint -- this is important in case we generate a divergent context from a potential divergent context
    std::vector<z3::expr> path_constraint = divergent_state->getPathConstraint();
    for (const z3::expr &constraint : divergent_state->getNewPathConstraint()) {
        path_constraint.push_back(constraint);
    }
    std::map<std::string, unsigned int> flattened_name_to_version = divergent_state->getFlattenedNameToVersion();
    std::unique_ptr<State> translated_state =
            std::make_unique<State>(vertex, std::move(concrete_valuations), std::move(symbolic_valuations),
                                    std::move(path_constraint), std::move(flattened_name_to_version));
    // translated context construction
    unsigned int cycle = divergent_context->getCycle();
    std::deque<std::shared_ptr<Frame>> call_stack = divergent_context->getCallStack();
    std::unique_ptr<Context> context =
            std::make_unique<Context>(cycle, std::move(translated_state), std::move(call_stack));
    return context;
}

bool Engine::isTerminationCriteriaMet() const {
    auto logger = spdlog::get("Shadow");
    bool termination_criteria_met = true;
    if (_termination_criteria & TerminationCriteria::TIME_OUT) {
        if (!isTimeOut()) {
            termination_criteria_met = false;
        } else {
            SPDLOG_LOGGER_TRACE(logger, "Termination criteria \"TIME_OUT\" met, terminating engine.");
            return true;
        }
    }
    if (_termination_criteria & TerminationCriteria::CYCLE_BOUND) {
        if (_cycle < _cycle_bound) {
            termination_criteria_met = false;
        } else {
            SPDLOG_LOGGER_TRACE(logger, "Termination criteria \"CYCLE_BOUND\" met, terminating engine.");
            return true;
        }
    }
    if (_termination_criteria & TerminationCriteria::COVERAGE) {
        constexpr float EPSILON = 0.01;
        if (std::abs(1 - _explorer->_branch_coverage) > EPSILON ||
            std::abs(1 - _explorer->_statement_coverage) > EPSILON) {
            termination_criteria_met = false;
        } else {
            SPDLOG_LOGGER_TRACE(logger, "Termination criteria \"COVERAGE\" met, terminating engine.");
            return true;
        }
    }
    return termination_criteria_met;
}

boost::optional<Engine::TerminationCriteria> Engine::isLocalTerminationCriteriaMet() const {
    auto logger = spdlog::get("Shadow");
    if (_termination_criteria & TerminationCriteria::TIME_OUT) {
        if (isTimeOut()) {
            SPDLOG_LOGGER_TRACE(logger, "Local termination criteria \"TIME_OUT\" met, terminating engine.");
            return TerminationCriteria::TIME_OUT;
        }
    }
    if (_termination_criteria & TerminationCriteria::COVERAGE) {
        constexpr float EPSILON = 0.01;
        if (std::abs(1 - _explorer->_branch_coverage) <= EPSILON &&
            std::abs(1 - _explorer->_statement_coverage) <= EPSILON) {
            SPDLOG_LOGGER_TRACE(logger, "Local termination criteria \"COVERAGE\" met, terminating engine.");
            return TerminationCriteria::COVERAGE;
        }
    }
    return boost::none;
}

bool Engine::isTimeOut() const {
    using namespace std::literals;
    auto elapsed_time = (std::chrono::system_clock::now() - _begin_time_point) / 1ms;
    if (elapsed_time < _time_out) {
        return false;
    } else {
        return true;
    }
}

void Engine::initializeBoundedExecution(const Cfg &cfg, const Context &context, const TestCase &test_case) {
    _cycle = context.getCycle();
    const std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                       std::vector<unsigned int>>>> &execution_history =
            test_case.getExecutionHistory();
    _explorer->initialize(cfg, context, execution_history);
    _executor->initialize(cfg);
    // XXX align global variable versioning with the divergent context for consistent execution
    State &state = context.getState();
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        unsigned int highest_version = state.getHighestVersion(flattened_name);
        _executor->setVersion(flattened_name, highest_version);
    }
    _merger->initialize(cfg);
}

std::unique_ptr<Context> Engine::initializeDivergenceExecution(const Cfg &cfg, const TestCase &test_case) {
    _divergence_executor->initialize(cfg);
    // initial state construction from test case
    const std::map<std::string, z3::expr> &initial_concrete_state_valuations =
            test_case.getInitialConcreteStateValuations();
    const Vertex &vertex = cfg.getEntry();
    std::map<std::string, z3::expr> concrete_valuations;
    std::map<std::string, std::pair<z3::expr, z3::expr>> concrete_shadow_valuations;
    std::map<std::string, z3::expr> symbolic_valuations;
    std::map<std::string, std::pair<z3::expr, z3::expr>> symbolic_shadow_valuations;
    std::map<std::string, unsigned int> flattened_name_to_version;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        flattened_name_to_version.emplace(flattened_name, 0);
        std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(_cycle);
        // distinguish between "whole-program" input variables and local/output variables
        if (_divergence_executor->isWholeProgramInput(flattened_name)) {
            // XXX input valuations get overwritten during execution (driven by the valuations of the test case)
            const ir::DataType &data_type = it->getDataType();
            z3::expr concrete_valuation = it->hasInitialization() ? _solver->makeValue(it->getInitialization())
                                                                  : _solver->makeDefaultValue(data_type);
            concrete_valuations.emplace(contextualized_name, concrete_valuation);
            z3::expr symbolic_valuation = _solver->makeConstant(contextualized_name, data_type);
            symbolic_valuations.emplace(contextualized_name, symbolic_valuation);
        } else {
            z3::expr concrete_valuation = initial_concrete_state_valuations.at(flattened_name);
            concrete_valuations.emplace(contextualized_name, concrete_valuation);
            symbolic_valuations.emplace(contextualized_name, concrete_valuation);
        }
    }
    std::vector<z3::expr> path_constraint{_solver->makeBooleanValue(true)};
    std::vector<z3::expr> old_path_constraint{_solver->makeBooleanValue(true)};
    std::vector<z3::expr> new_path_constraint{_solver->makeBooleanValue(true)};
    std::unique_ptr<DivergentState> divergent_state = std::make_unique<DivergentState>(
            vertex, std::move(concrete_valuations), std::move(concrete_shadow_valuations),
            std::move(symbolic_valuations), std::move(symbolic_shadow_valuations), std::move(path_constraint),
            std::move(old_path_constraint), std::move(new_path_constraint), std::move(flattened_name_to_version));
    // initial context construction
    unsigned int cycle = _cycle;
    std::deque<std::shared_ptr<Frame>> call_stack{std::make_shared<Frame>(cfg, cfg.getName(), cfg.getEntryLabel())};
    std::unique_ptr<Context> context =
            std::make_unique<Context>(cycle, std::move(divergent_state), std::move(call_stack));
    return context;
}

std::pair<std::unique_ptr<Context>, boost::optional<Engine::TerminationCriteria>>
Engine::stepUntilBound(std::vector<std::shared_ptr<TestCase>> &test_cases) {
    auto logger = spdlog::get("Shadow");
    std::unique_ptr<Context> succeeding_cycle_context;

    while (!_explorer->isEmpty() || !_merger->isEmpty()) {
        SPDLOG_LOGGER_TRACE(logger, "Explorer:\n{}", *_explorer);
        SPDLOG_LOGGER_TRACE(logger, "Merger:\n{}", *_merger);
        boost::optional<TerminationCriteria> reason = isLocalTerminationCriteriaMet();
        if (reason.has_value()) {
            return std::make_pair(nullptr, reason);
        }
        if (_explorer->isEmpty()) {
            assert(!_merger->isEmpty());
            std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();
            std::unique_ptr<Context> merged_context = _merger->merge();
            using namespace std::literals;
            auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
            SPDLOG_LOGGER_TRACE(logger, "Merging took {}ms, resulting merged context:\n{}", elapsed_time,
                                *merged_context);
            _explorer->push(std::move(merged_context));
        } else {
            std::unique_ptr<Context> context = _explorer->pop();
            const State &state = context->getState();
            const Vertex &vertex = state.getVertex();
            unsigned int label = vertex.getLabel();
            std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>> succeeding_contexts =
                    _executor->execute(std::move(context));
            std::unique_ptr<Context> succeeding_context = std::move(succeeding_contexts.first);
            std::pair<bool, bool> coverage = _explorer->updateCoverage(label, *succeeding_context);
            if (coverage.second) {
                SPDLOG_LOGGER_TRACE(logger,
                                    "Branch coverage has been increased, deriving test case from succeeding context.");
                std::shared_ptr<TestCase> test_case = _test_suite->deriveTestCase(*succeeding_context);
                if (test_case) {
                    auto simulator = std::make_unique<simulator::Engine>(
                            simulator::Evaluator::ShadowProcessingMode::OLD, *_solver);
                    auto simulation_result = simulator->run(succeeding_context->getMainFrame().getCfg(),
                                                            test_case->getInitialConcreteStateValuations(),
                                                            test_case->getCycleToConcreteInputValuations());
                    std::map<unsigned int,
                             std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                   std::vector<unsigned int>>>>
                            execution_history = simulation_result.first;
                    std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_output_valuations =
                            simulation_result.second;
                    // XXX augment test case with additional information such as execution history and state valuations
                    // XXX reaching cycle end of the old program version
                    test_case->setExecutionHistory(execution_history);
                    test_case->setCycleToConcreteStateValuations(cycle_to_concrete_output_valuations);
                    test_cases.push_back(test_case);
                }
            }
            if (_cycle == succeeding_context->getCycle()) {
                if (_merger->reachedMergePoint(*succeeding_context)) {
                    SPDLOG_LOGGER_TRACE(logger, "Succeeding context reached a merge point.");
                    _merger->push(std::move(succeeding_context));
                } else {
                    _explorer->push(std::move(succeeding_context));
                }
                if (succeeding_contexts.second.has_value()) {
                    std::unique_ptr<Context> forked_succeeding_context = std::move(*succeeding_contexts.second);
                    coverage = _explorer->updateCoverage(label, *forked_succeeding_context);
                    if (coverage.second) {
                        SPDLOG_LOGGER_TRACE(
                                logger, "Branch coverage has been increased, deriving test case from forked succeeding "
                                        "context.");
                        std::shared_ptr<TestCase> test_case = _test_suite->deriveTestCase(*forked_succeeding_context);
                        if (test_case) {
                            auto simulator = std::make_unique<simulator::Engine>(
                                    simulator::Evaluator::ShadowProcessingMode::OLD, *_solver);
                            auto simulation_result = simulator->run(forked_succeeding_context->getMainFrame().getCfg(),
                                                                    test_case->getInitialConcreteStateValuations(),
                                                                    test_case->getCycleToConcreteInputValuations());
                            std::map<unsigned int,
                                     std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                           std::vector<unsigned int>>>>
                                    execution_history = simulation_result.first;
                            std::map<unsigned int, std::map<std::string, z3::expr>>
                                    cycle_to_concrete_output_valuations = simulation_result.second;
                            // XXX augment test case with additional information such as execution history and state
                            // XXX valuations reaching cycle end of the old program version
                            test_case->setExecutionHistory(execution_history);
                            test_case->setCycleToConcreteStateValuations(cycle_to_concrete_output_valuations);
                            test_cases.push_back(test_case);
                        }
                    }
                    assert(_cycle == forked_succeeding_context->getCycle());
                    if (_merger->reachedMergePoint(*forked_succeeding_context)) {
                        SPDLOG_LOGGER_TRACE(logger, "Forked succeeding context reached a merge point.");
                        _merger->push(std::move(forked_succeeding_context));
                    } else {
                        _explorer->push(std::move(forked_succeeding_context));
                    }
                }
            } else {
                assert(_cycle < succeeding_context->getCycle());
                assert(!succeeding_contexts.second.has_value());
                assert(!_merger->reachedMergePoint(*succeeding_context));
                succeeding_cycle_context = std::move(succeeding_context);
            }
        }
    }
    _cycle = _cycle + 1;
    return std::make_pair(std::move(succeeding_cycle_context), boost::none);
}

std::pair<DivergenceExecutor::ExecutionStatus, std::vector<std::unique_ptr<Context>>>
Engine::stepUntilDivergence(Context &context) {
    while (context.getCycle() == _cycle) {
        auto result = _divergence_executor->execute(context);
        DivergenceExecutor::ExecutionStatus execution_status = result.first;
        if (execution_status == DivergenceExecutor::ExecutionStatus::DIVERGENT_BEHAVIOR ||
            execution_status == DivergenceExecutor::ExecutionStatus::POTENTIAL_DIVERGENT_BEHAVIOR) {
            return result;
        } else {
            // XXX continue execution
            assert(execution_status == DivergenceExecutor::ExecutionStatus::EXPECTED_BEHAVIOR);
        }
    }
    _cycle = _cycle + 1;
    return std::make_pair(DivergenceExecutor::ExecutionStatus::EXPECTED_BEHAVIOR,
                          std::vector<std::unique_ptr<Context>>());
}

std::unique_ptr<TestCase> Engine::fromXML(const std::string &file_path) {
    assert(boost::filesystem::is_regular_file(file_path));
    boost::property_tree::ptree property_tree;
    boost::property_tree::read_xml(file_path, property_tree);
    std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_input_valuations;
    std::map<std::string, z3::expr> initial_concrete_state_valuations;
    for (const boost::property_tree::ptree::value_type &test_case_property : property_tree.get_child("testcase")) {
        if (test_case_property.first == "initialization") {
            for (const boost::property_tree::ptree::value_type &valuation_property : test_case_property.second) {
                auto variable = valuation_property.second.get<std::string>("<xmlattr>.variable");
                std::string value = valuation_property.second.data();
                if (value == "true") {
                    initial_concrete_state_valuations.emplace(variable, _solver->makeBooleanValue(true));
                } else if (value == "false") {
                    initial_concrete_state_valuations.emplace(variable, _solver->makeBooleanValue(false));
                } else {
                    initial_concrete_state_valuations.emplace(variable, _solver->makeIntegerValue(std::stoi(value)));
                }
            }
        } else if (test_case_property.first == "input") {
            auto cycle = test_case_property.second.get<unsigned int>("<xmlattr>.cycle");
            std::map<std::string, z3::expr> concrete_input_valuations;
            for (const boost::property_tree::ptree::value_type &input_property : test_case_property.second) {
                if (input_property.first == "<xmlattr>") {
                    // XXX do nothing
                    continue;
                } else if (input_property.first == "valuation") {
                    auto variable = input_property.second.get<std::string>("<xmlattr>.variable");
                    std::string value = input_property.second.data();
                    if (value == "true") {
                        concrete_input_valuations.emplace(variable, _solver->makeBooleanValue(true));
                    } else if (value == "false") {
                        concrete_input_valuations.emplace(variable, _solver->makeBooleanValue(false));
                    } else {
                        concrete_input_valuations.emplace(variable, _solver->makeIntegerValue(std::stoi(value)));
                    }
                } else {
                    throw std::runtime_error("Unexpected input property.");
                }
            }
            cycle_to_concrete_input_valuations.emplace(cycle, std::move(concrete_input_valuations));
        } else {
            throw std::runtime_error("Unexpected test case property.");
        }
    }
    return std::make_unique<TestCase>(std::move(initial_concrete_state_valuations),
                                      std::move(cycle_to_concrete_input_valuations));
}

void Engine::reset() {
    _cycle = 0;
    _explorer->reset();
    _divergence_executor->reset();
    _executor->reset();
    _merger->reset();
    // XXX do not reset the test suite in order to prevent the addition of duplicate test cases during bounded execution
    // _test_suite->reset();
}

bool Engine::containsShadowExpression(const std::map<std::string, z3::expr> &valuations,
                                      const std::map<std::string, std::pair<z3::expr, z3::expr>> &shadow_valuations,
                                      const z3::expr &expression) const {
    // XXX check if expression is already a shadow expression, else check if it contains shadow expressions
    if (shadow_valuations.find(expression.to_string()) != shadow_valuations.end()) {
        return true;
    } else {
        std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
        if (uninterpreted_constants.empty()) {
            assert(expression.is_true() || expression.is_false() || expression.is_numeral());
            return false;
        } else if (uninterpreted_constants.size() == 1) {
            std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
            const z3::expr &nested_expression = valuations.at(contextualized_name);
            std::vector<z3::expr> nested_uninterpreted_constants =
                    _solver->getUninterpretedConstants(nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
                return false;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    return shadow_valuations.find(nested_contextualized_name) != shadow_valuations.end();
                } else {
                    return containsShadowExpression(valuations, shadow_valuations, nested_expression);
                }
            } else {
                return containsShadowExpression(valuations, shadow_valuations, nested_expression);
            }
        } else {
            bool contains_shadow_expression = false;
            for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
                if (containsShadowExpression(valuations, shadow_valuations, uninterpreted_constant)) {
                    contains_shadow_expression = true;
                    break;
                } else {
                    std::string contextualized_name = uninterpreted_constant.decl().name().str();
                    const z3::expr &nested_expression = valuations.at(contextualized_name);
                    if (containsShadowExpression(valuations, shadow_valuations, nested_expression)) {
                        contains_shadow_expression = true;
                        break;
                    }
                }
            }
            return contains_shadow_expression;
        }
    }
}

z3::expr Engine::lowerToShadowExpression(const std::map<std::string, z3::expr> &valuations,
                                         const std::map<std::string, std::pair<z3::expr, z3::expr>> &shadow_valuations,
                                         const z3::expr &expression) const {
    // XXX check if expression is already a shadow expression, else check if it contains shadow expressions
    if (shadow_valuations.find(expression.to_string()) != shadow_valuations.end()) {
        // XXX return new concrete valuations
        return shadow_valuations.at(expression.to_string()).second;
    } else {
        std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
        if (uninterpreted_constants.empty()) {
            assert(expression.is_true() || expression.is_false() || expression.is_numeral());
            return expression;
        } else if (uninterpreted_constants.size() == 1) {
            std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
            const z3::expr &nested_expression = valuations.at(contextualized_name);
            std::vector<z3::expr> nested_uninterpreted_constants =
                    _solver->getUninterpretedConstants(nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
                return expression;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    return expression;
                } else {
                    return lowerToShadowExpression(valuations, shadow_valuations, nested_expression);
                }
            } else {
                return lowerToShadowExpression(valuations, shadow_valuations, nested_expression);
            }
        } else {
            z3::expr lowered_expression = expression;
            for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
                z3::expr lowered_uninterpreted_constant =
                        lowerToShadowExpression(valuations, shadow_valuations, uninterpreted_constant);
                z3::expr_vector source(_solver->getContext());
                source.push_back(uninterpreted_constant);
                z3::expr_vector destination(_solver->getContext());
                destination.push_back(lowered_uninterpreted_constant);
                lowered_expression = lowered_expression.substitute(source, destination);
            }
            return lowered_expression.simplify();
        }
    }
}