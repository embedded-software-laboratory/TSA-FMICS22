#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "se/configuration.h"
#include "se/shadow/engine.h"
#include "se/simulator/engine.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeSimulator : public ::testing::Test {
public:
    TestLibSeSimulator() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Simulator");
    }

    void createLogger(const std::string &name) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + name + ".txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::info);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Simulator");
    }

    static std::shared_ptr<Cfg> getCfg(std::string const &path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        const std::string &source_code = buffer.str();
        auto project = Compiler::compile(source_code);
        auto builder = std::make_unique<Builder>(*project);
        return builder->build();
    }
};

TEST_F(TestLibSeSimulator, Scenario3_Final) {
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PPU/ST/Scenario3_Final.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 5;
    auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
    engine->_test_suite->fromXML("../../test/benchmark/dissertation/PPU/ST/test_suite_scenario_3");
    auto simulator = std::make_unique<se::simulator::Engine>(se::simulator::Evaluator::ShadowProcessingMode::NEW,
                                                             *engine->_solver);
    simulator->run(*cfg, engine->_test_suite->getTestCases());
    std::cout << *simulator->_explorer << std::endl;
    ASSERT_EQ(0, 1);
}