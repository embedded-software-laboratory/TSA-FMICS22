#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "se/baseline/engine.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeBaseline : public ::testing::Test {
public:
    TestLibSeBaseline() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/baseline.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("Baseline", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Baseline");
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

TEST_F(TestLibSeBaseline, Call_Coverage_In_3_Cycles_Cfg) {
    using namespace se::baseline;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_coverage_in_3_cycles.st");
    auto engine = std::make_unique<Engine>();
    engine->initialize(*cfg);
    while (engine->_cycle < 3) {
        std::vector<std::unique_ptr<Context>> succeeding_cycle_contexts = engine->step();
        std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
        for (std::unique_ptr<Context> &succeeding_cycle_context : succeeding_cycle_contexts) {
            std::cout << *succeeding_cycle_context << std::endl;
            engine->_explorer->pushContext(std::move(succeeding_cycle_context));
        }
        std::cout << "\n###################################################\n\n" << std::endl;
    }
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeBaseline, Benchmark_Antivalent) {
    using namespace se::baseline;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/antivalentX.st");
    auto engine = std::make_unique<Engine>();
    engine->initialize(*cfg);
    std::vector<std::unique_ptr<Context>> succeeding_cycle_contexts = engine->step();
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_cycle_context : succeeding_cycle_contexts) {
        std::cout << *succeeding_cycle_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_cycle_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    ASSERT_EQ(0, 1);
}