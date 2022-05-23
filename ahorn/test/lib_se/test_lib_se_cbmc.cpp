#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "se/cbmc/engine.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeCBMC : public ::testing::Test {
public:
    TestLibSeCBMC() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/cbmc.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("CBMC", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("CBMC");
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

TEST_F(TestLibSeCBMC, Call_Coverage_In_3_Cycles_Cfg) {
    using namespace se::cbmc;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_coverage_in_3_cycles.st");
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 20;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeCBMC, Benchmark_Dissertation_PLCopen_safety_SFAntivalent) {
    using namespace se::cbmc;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFAntivalent.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 10;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}