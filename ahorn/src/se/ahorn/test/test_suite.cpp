#include "se/ahorn/test/test_suite.h"
#include "se/utilities/fmt_formatter.h"

#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <chrono>

using namespace se::ahorn;

TestSuite::TestSuite(Solver &solver) : _solver(&solver), _test_cases(std::vector<std::unique_ptr<TestCase>>()) {}

void TestSuite::deriveTestCase(const Context &context) {
    auto logger = spdlog::get("Ahorn");

    const Frame &frame = context.getMainFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::Interface &interface = cfg.getInterface();
    const State &state = context.getState();

    std::map<std::string, z3::expr> initial_concrete_state_valuations;
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        auto input_variable_it = std::find_if(interface.inputVariablesBegin(), interface.inputVariablesEnd(),
                                              [&flattened_name](const auto &input_variable) {
                                                  return flattened_name == input_variable.getFullyQualifiedName();
                                              });
        if (input_variable_it == interface.inputVariablesEnd()) {
            std::string contextualized_name = flattened_name + "_" + std::to_string(0) + "__" + std::to_string(0);
            initial_concrete_state_valuations.emplace(flattened_name, state.getConcreteValuation(contextualized_name));
        }
    }

    std::map<unsigned int, std::map<std::string, z3::expr>> cycle_to_concrete_input_valuations;
    for (unsigned int cycle = 0; cycle <= context.getCycle(); ++cycle) {
        cycle_to_concrete_input_valuations.emplace(cycle, std::map<std::string, z3::expr>());
        for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
            std::string flattened_name = it->getFullyQualifiedName();
            // XXX handling of program exit ensures that all initial valuations of a cycle start at version 0
            unsigned int lowest_version = 0;
            std::string contextualized_name =
                    flattened_name + "_" + std::to_string(lowest_version) + "__" + std::to_string(cycle);
            cycle_to_concrete_input_valuations.at(cycle).emplace(flattened_name,
                                                                 state.getConcreteValuation(contextualized_name));
        }
    }

    // XXX copy due to debug output below
    std::unique_ptr<TestCase> test_case =
            std::make_unique<TestCase>(initial_concrete_state_valuations, cycle_to_concrete_input_valuations);

    // XXX check if there already exists a test case with the same initial concrete state valuation and concrete
    // XXX input valuation. This can occur if the same valuation covers multiple branches through one execution,
    // XXX because the engine tries to derive a test case whenever the coverage value changes.
    auto it = std::find_if(_test_cases.begin(), _test_cases.end(),
                           [&test_case](const std::unique_ptr<TestCase> &existing_test_case) {
                               return *test_case == *existing_test_case;
                           });
    if (it == _test_cases.end()) {
        SPDLOG_LOGGER_TRACE(logger, "Initial concrete state valuations of derived test case:\n{}",
                            initial_concrete_state_valuations);

        SPDLOG_LOGGER_TRACE(logger, "Concrete input valuations of derived test case:\n{}",
                            cycle_to_concrete_input_valuations);
        _test_cases.push_back(std::move(test_case));
    } else {
        SPDLOG_LOGGER_TRACE(logger, "A test case with the same valuation has already been created, skipping "
                                    "augmentation of test suite.");
    }
}

void TestSuite::toXML(const std::string &path) const {
    using namespace std::chrono;
    const auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::string path_prefix = path.empty() ? "test_suite_" + std::to_string(ms) : path;
    if (boost::filesystem::create_directory(path_prefix)) {
        for (std::vector<std::unique_ptr<TestCase>>::size_type i = 0; i < _test_cases.size(); ++i) {
            toXML(*_test_cases.at(i), path_prefix + "/" + "test_case_" + std::to_string(i) + ".xml");
        }
    } else {
        if (boost::filesystem::exists(path_prefix)) {
            for (std::vector<std::unique_ptr<TestCase>>::size_type i = 0; i < _test_cases.size(); ++i) {
                toXML(*_test_cases.at(i), path_prefix + "/" + "test_case_" + std::to_string(i) + ".xml");
            }
        } else {
            throw std::runtime_error("Could not create directory at path " + path_prefix);
        }
    }
}

void TestSuite::fromXML(const std::string &path) {
    if (boost::filesystem::is_directory(path)) {
        for (const auto &directory_iterator : boost::filesystem::directory_iterator(path)) {
            const std::string &file_path = directory_iterator.path().string();
            if (file_path.find(".xml") != std::string::npos) {
                std::unique_ptr<TestCase> test_case = buildFromXML(file_path);
                _test_cases.push_back(std::move(test_case));
            }
        }
    } else {
        std::unique_ptr<TestCase> test_case = buildFromXML(path);
        _test_cases.push_back(std::move(test_case));
    }
}

void TestSuite::toXML(const TestCase &test_case, const std::string &path) const {
    boost::property_tree::ptree property_tree;
    boost::property_tree::ptree &test_case_node = property_tree.add("testcase", "");
    boost::property_tree::ptree &initialization_node = test_case_node.add("initialization", "");
    for (const auto &initial_concrete_state_valuation : test_case.getInitialConcreteStateValuations()) {
        boost::property_tree::ptree &valuation_node =
                initialization_node.add("valuation", initial_concrete_state_valuation.second);
        valuation_node.put("<xmlattr>.variable", initial_concrete_state_valuation.first);
    }
    for (const auto &cycle_to_concrete_input_valuations : test_case.getCycleToConcreteInputValuations()) {
        unsigned int cycle = cycle_to_concrete_input_valuations.first;
        boost::property_tree::ptree &input_node = test_case_node.add("input", "");
        input_node.put("<xmlattr>.cycle", cycle);
        for (const auto &concrete_input_valuation : cycle_to_concrete_input_valuations.second) {
            boost::property_tree::ptree &valuation_node = input_node.add("valuation", concrete_input_valuation.second);
            valuation_node.put("<xmlattr>.variable", concrete_input_valuation.first);
        }
    }
    boost::property_tree::write_xml(path, property_tree);
}

std::unique_ptr<TestCase> TestSuite::buildFromXML(const std::string &path) const {
    assert(boost::filesystem::is_regular_file(path));
    boost::property_tree::ptree property_tree;
    boost::property_tree::read_xml(path, property_tree);
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