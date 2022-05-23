#include "se/experimental/test_suite.h"
#include "utilities/ostream_operators.h"

#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <chrono>
#include <map>
#include <sstream>

using namespace se;

TestSuite::TestSuite(Manager &manager) : _manager(&manager) {}

std::ostream &TestSuite::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tsize: " << _test_cases.size() << "\n";
    str << ")";
    return os << str.str();
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
        throw std::runtime_error("Could not create directory at path " + path_prefix);
    }
}

void TestSuite::toXML(const TestCase &test_case, const std::string &path) const {
    boost::property_tree::ptree property_tree;
    boost::property_tree::ptree &test_case_node = property_tree.add("testcase", "");
    for (const auto &cycle_to_inputs : test_case.getInputs()) {
        unsigned int cycle = cycle_to_inputs.first;
        boost::property_tree::ptree &input_node = test_case_node.add("input", "");
        input_node.put("<xmlattr>.cycle", cycle);
        for (const auto &input_valuation : cycle_to_inputs.second) {
            boost::property_tree::ptree &valuation_node = input_node.add("valuation", input_valuation.second);
            valuation_node.put("<xmlattr>.variable", input_valuation.first);
        }
    }
    boost::property_tree::write_xml(path, property_tree);
}

std::unique_ptr<TestCase> TestSuite::fromXML(const std::string &path) const {
    boost::property_tree::ptree property_tree;
    boost::property_tree::read_xml(path, property_tree);
    std::map<unsigned int, std::map<std::string, z3::expr>> inputs;
    for (const boost::property_tree::ptree::value_type &test_case_property : property_tree.get_child("testcase")) {
        if (test_case_property.first == "input") {
            auto cycle = test_case_property.second.get<unsigned int>("<xmlattr>.cycle");
            std::map<std::string, z3::expr> valuations;
            for (const boost::property_tree::ptree::value_type &input_property : test_case_property.second) {
                if (input_property.first == "<xmlattr>") {
                    // XXX do nothing
                    continue;
                } else if (input_property.first == "valuation") {
                    auto variable = input_property.second.get<std::string>("<xmlattr>.variable");
                    std::string value = input_property.second.data();
                    if (value == "true") {
                        valuations.emplace(variable, _manager->makeBooleanValue(true));
                    } else if (value == "false") {
                        valuations.emplace(variable, _manager->makeBooleanValue(false));
                    } else {
                        valuations.emplace(variable, _manager->makeIntegerValue(std::stoi(value)));
                    }
                } else {
                    throw std::runtime_error("Unexpected input property.");
                }
            }
            inputs.emplace(cycle, std::move(valuations));
        } else {
            throw std::runtime_error("Unexpected test case property.");
        }
    }
    return std::make_unique<TestCase>(
            std::move(inputs), std::map<unsigned int, std::map<std::string, std::shared_ptr<ConcreteExpression>>>());
}

const std::vector<std::unique_ptr<TestCase>> &TestSuite::getTestCases() const {
    return _test_cases;
}

// TODO (25.01.2022): In case divergent behavior is possible, test cases must be created. I want to save them in a
//  different container, rather than putting them in the test suite. Change this function (and maybe refactor to a
//  different class?)
void TestSuite::createTestCase(const Context &context) {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Creating test case from context: {}", context);

    const Frame &main_frame = context.getMainFrame();
    const Cfg &main_cfg = main_frame.getCfg();
    const ir::Interface &interface = main_cfg.getInterface();

    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();
    const State &state = context.getState();
    const Vertex &vertex = state.getVertex();

    std::map<std::string, std::string> input_valuations;
    std::map<unsigned int, std::map<std::string, z3::expr>> inputs;
    for (unsigned int cycle = 0; cycle <= context.getCycle(); ++cycle) {
        inputs.emplace(cycle, std::map<std::string, z3::expr>());
        for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
            std::string flattened_name = it->getFullyQualifiedName();
            unsigned int minimum_version = state.getMinimumVersionFromConcreteStore(flattened_name, cycle);
            std::string contextualized_name =
                    flattened_name + "_" + std::to_string(minimum_version) + "__" + std::to_string(cycle);
            std::shared_ptr<Expression> expression = state.getConcreteExpression(contextualized_name);
            inputs.at(cycle).emplace(flattened_name, expression->getZ3Expression());
            input_valuations.emplace(contextualized_name, expression->getZ3Expression().to_string());
        }
    }

    SPDLOG_LOGGER_TRACE(logger, "Input valuations:");
    for (const auto &p : input_valuations) {
        SPDLOG_LOGGER_TRACE(logger, "{} -> {}", p.first, p.second);
    }

    // TODO (12.01.2022): Auf welchen Pfad bezieht sich der Testcase? -> Kann man etwas aus der Execution History
    //  ziehen?
    /*
    SPDLOG_LOGGER_TRACE(logger, "Assumption literals:");
    for (const auto &assumption_literals : state.getAssumptionLiterals()) {
        SPDLOG_LOGGER_TRACE(logger, "{} -> {}", assumption_literals.first, assumption_literals.second);
    }
     */
    std::map<unsigned int, std::map<std::string, std::shared_ptr<ConcreteExpression>>> response;
    _test_cases.push_back(std::make_unique<TestCase>(std::move(inputs), std::move(response)));
}
