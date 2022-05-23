#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "se/oa/engine.h"
#include "se/configuration.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeOa : public ::testing::Test {
public:
    TestLibSeOa() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/oa.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("Oa", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::info);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Oa");
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

TEST_F(TestLibSeOa, Call_Coverage_In_3_Cycles) {
    using namespace se::oa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/call_coverage_in_3_cycles.st");
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeOa, Unreachable_Branch_In_Call) {
    using namespace se::oa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/unreachable_branch_in_call.st");
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeOa, SF_Antivalent) {
    using namespace se::oa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFAntivalent.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}