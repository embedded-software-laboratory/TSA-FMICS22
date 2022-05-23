#include "boost/program_options.hpp"
#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "pass/basic_block_pass.h"
#include "pass/call_transformation_pass.h"
#include "pass/ssa/ssa_pass.h"
#include "pass/tac/tac_pass.h"
#include "sa/analyzer.h"
#include "se/ahorn/engine.h"
#include "se/cbmc/engine.h"
#include "se/configuration.h"
#include "se/shadow/engine.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

std::shared_ptr<Cfg> getCfg(const std::string &file_path) {
    std::ifstream file(file_path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    const std::string &source_code = buffer.str();
    auto project = Compiler::compile(source_code);
    auto builder = std::make_unique<Builder>(*project);
    return builder->build();
}

void toDot(const Cfg &cfg, const std::string &file_path) {
    std::ofstream file(file_path);
    file << cfg.toDot();
    file.flush();
    file.close();
}

void createLogger(const std::string &name, spdlog::level::level_enum level) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + name + ".txt", true);
    file_sink->set_level(level);

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(console_sink);
    sinks.push_back(file_sink);

    auto logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
    logger->set_level(level);

    spdlog::register_logger(logger);
}

int main(int argc, char *argv[]) {
    boost::program_options::options_description command_line_options;

    boost::program_options::options_description generic_options("Generic options");
    generic_options.add_options()("help,h", "produce help message")("version", "print version string")(
            "verbose", boost::program_options::value<std::string>(), "output for diagnostic purpose [trace | info]")(
            "input-file,i", boost::program_options::value<std::string>(),
            "input file")("to-dot", boost::program_options::value<std::string>(),
                          "generates a dot representation of the cfg to file")(
            "cycle-bound", boost::program_options::value<unsigned>(), "cycle bound (default = 10)")(
            "time-out,t", boost::program_options::value<long>(), "time-out in milliseconds (default = 10000ms)")(
            "unreachable-labels", boost::program_options::value<std::vector<unsigned int>>()->multitoken(),
            "unreachable labels")("unreachable-branches",
                                  boost::program_options::value<std::vector<std::string>>()->multitoken(),
                                  "unreachable branches");

    boost::program_options::options_description static_analysis("Static analysis");
    static_analysis.add_options()("sa", "performs static analysis on the cfg");

    boost::program_options::options_description compositional_symbolic_execution("Compositional symbolic execution");
    compositional_symbolic_execution.add_options()("cse", "performs compositional symbolic execution on the cfg")(
            "exploration-strategy,es", boost::program_options::value<std::string>(),
            "exploration strategy [depth-first | breadth-first]")("generate-test-suite",
                                                                  boost::program_options::value<std::string>(),
                                                                  "generates a test suite for the cfg to directory");

    boost::program_options::options_description shadow_symbolic_execution("Shadow symbolic execution");
    shadow_symbolic_execution.add_options()("sse", "performs shadow symbolic execution on the cfg and given test "
                                                   "suite")(
            "test-suite,ts", boost::program_options::value<std::string>(), "file path to test suite directory");

    boost::program_options::options_description cycle_bounded_model_checking_encoding(
            "Cycle-bounded model checking encoding");
    cycle_bounded_model_checking_encoding.add_options()("cbmc",
                                                        "performs symbolic execution using CBMC encoding on the cfg");

    command_line_options.add(generic_options)
            .add(static_analysis)
            .add(compositional_symbolic_execution)
            .add(shadow_symbolic_execution)
            .add(cycle_bounded_model_checking_encoding);

    boost::program_options::positional_options_description positionals;
    positionals.add("input-file", -1);

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv)
                                          .options(command_line_options)
                                          .positional(positionals)
                                          .run(),
                                  vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << command_line_options << std::endl;
        return 0;
    }

    if (!vm.count("input-file")) {
        std::cout << "No input file provided." << std::endl;
        return 1;
    }

    std::shared_ptr<Cfg> cfg = getCfg(vm["input-file"].as<std::string>());

    if (vm.count("to-dot")) {
        toDot(*cfg, vm["to-dot"].as<std::string>());
    }

    spdlog::level::level_enum level = spdlog::level::off;
    if (vm.count("verbose")) {
        if (vm["verbose"].as<std::string>() == "trace") {
            level = spdlog::level::trace;
        } else if (vm["verbose"].as<std::string>() == "info") {
            level = spdlog::level::info;
        } else {
            std::cout << "Invalid verbosity level provided." << std::endl;
            return 1;
        }
    }

    if (vm.count("sa")) {
        createLogger("Tac", level);
        createLogger("Static Analysis", level);
        std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
        std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
        std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
        std::unique_ptr<pass::CallTransformationPass> ct_pass = std::make_unique<pass::CallTransformationPass>();
        std::shared_ptr<Cfg> ct_cfg = ct_pass->apply(*tac_cfg);
        std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
        std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*ct_cfg);
        std::unique_ptr<sa::Analyzer> analyzer = std::make_unique<sa::Analyzer>();
        std::cout << "Performing value set analysis..." << std::endl;
        std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
        std::cout << "Unreachable labels:" << std::endl;
        for (const auto &unreachable_label : unreachable_labels) {
            std::cout << "label: " << unreachable_label << std::endl;
        }
    }

    std::unique_ptr<se::Configuration> configuration = std::make_unique<se::Configuration>();
    if (vm.count("cycle-bound")) {
        configuration->_cycle_bound = (vm["cycle-bound"].as<unsigned>());
    }
    if (vm.count("time-out")) {
        configuration->_time_out = (vm["time-out"].as<long>());
    }
    if (vm.count("unreachable-labels")) {
        std::vector<unsigned int> unreachable_labels = vm["unreachable-labels"].as<std::vector<unsigned int>>();
        if (!unreachable_labels.empty()) {
            configuration->_unreachable_information = true;
            configuration->_unreachable_labels = unreachable_labels;
        }
    }
    if (vm.count("unreachable-branches")) {
        std::vector<std::string> unreachable_branches = vm["unreachable-branches"].as<std::vector<std::string>>();
        if (!unreachable_branches.empty()) {
            std::map<unsigned int, std::pair<bool, bool>> translated_unreachable_branches;
            for (const std::string &unreachable_branch : unreachable_branches) {
                // 90_ff
                std::size_t pos = unreachable_branch.find('_');
                unsigned int label = std::stoi(unreachable_branch.substr(0, pos));
                bool branch = unreachable_branch.substr(pos + 1, unreachable_branch.size()) == "tt" ? true : false;
                if (translated_unreachable_branches.count(label)) {
                    if (branch) {
                        translated_unreachable_branches.at(label).first = true;
                    } else {
                        translated_unreachable_branches.at(label).second = true;
                    }
                } else {
                    translated_unreachable_branches.emplace(label,
                                                            branch ? std::pair(true, false) : std::pair(false, true));
                }
            }
            configuration->_unreachable_information = true;
            configuration->_unreachable_branches = translated_unreachable_branches;
        }
    }
    if (vm.count("generate-test-suite")) {
        configuration->_generate_test_suite = (vm["generate-test-suite"].as<std::string>());
    }
    if (vm.count("cse")) {
        createLogger("Ahorn", level);
        auto engine = std::make_unique<se::ahorn::Engine>(std::move(configuration));
        engine->run(*cfg);
    } else if (vm.count("sse")) {
        if (!vm.count("test-suite")) {
            std::cout << "No file path to test suite directory provided." << std::endl;
            return 1;
        }
        createLogger("Shadow", level);
        createLogger("Simulator", level);
        auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
        engine->run(*cfg, vm["test-suite"].as<std::string>());
    } else if (vm.count("cbmc")) {
        createLogger("CBMC", level);
        auto engine = std::make_unique<se::cbmc::Engine>(std::move(configuration));
        engine->run(*cfg);
    }

    return 0;
}
