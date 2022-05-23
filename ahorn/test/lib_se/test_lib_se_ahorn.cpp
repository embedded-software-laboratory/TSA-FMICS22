#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "se/ahorn/engine.h"
#include "se/configuration.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeAhorn : public ::testing::Test {
public:
    TestLibSeAhorn() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Ahorn");
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
        spdlog::drop("Ahorn");
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

TEST_F(TestLibSeAhorn, Toy_Example_Kuchta_et_al) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/toy_example_kuchta_et_al.st");
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Call_Coverage_In_3_Cycles) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/call_coverage_in_3_cycles.st");
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Call_Coverage_In_2_Cycles) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/call_coverage_in_2_cycles.st");
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_SFAntivalent) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFAntivalent.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_SFGuardMonitoring) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFGuardMonitoring.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_SFModeSelector) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFModeSelector.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_SFEDM) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFEDM.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_SFTestableSafetySensor) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SFTestableSafetySensor.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_DiagnosticsConcept) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg =
            getCfg("../../test/benchmark/dissertation/PLCopen_safety/examples/4.1.DiagnosticsConcept.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PLCopen_safety_TwoHandControl) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg =
            getCfg("../../test/benchmark/dissertation/PLCopen_safety/examples/4.5.TwoHandControl.st");
    std::cout << cfg->toDot() << std::endl;
    auto engine = std::make_unique<Engine>(std::make_unique<se::Configuration>());
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PPU_ST_Scenario0_Final) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PPU/ST/Scenario0_Final.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 25;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PPU_ST_Scenario1_Final) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PPU/ST/Scenario1_Final.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 25;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeAhorn, Benchmark_Dissertation_PPU_ST_Scenario3_Final) {
    using namespace se::ahorn;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PPU/ST/Scenario3_Final.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 20;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg);
    ASSERT_EQ(0, 1);
}