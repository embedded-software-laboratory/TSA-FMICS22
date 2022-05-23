#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "pass/change_annotation_collection_pass.h"
#include "se/ahorn/engine.h"
#include "se/shadow/engine.h"
#include "se/simulator/engine.h"

#include "boost/filesystem.hpp"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSeShadow : public ::testing::Test {
public:
    TestLibSeShadow() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Ahorn");
        createLogger("Shadow");
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
        spdlog::drop("Ahorn");
        spdlog::drop("Shadow");
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

TEST_F(TestLibSeShadow, Toy_Example_Kuchta_et_al_With_Change_Annotation) {
    using namespace se::shadow;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_se/file/toy_example_kuchta_et_al_with_change_annotation.st");
    std::cout << cfg->toDot() << std::endl;
    auto configuration = std::make_unique<se::Configuration>();
    configuration->_cycle_bound = 10;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    engine->run(*cfg, "../../test/lib_se/test_suite/toy_example_kuchta_et_al");

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeShadow, Augmentation_01) {
    std::shared_ptr<Cfg> shadow_cfg = getCfg("../../test/benchmark/dissertation/augmentation/01/S/S.st");
    std::cout << shadow_cfg->toDot() << std::endl;
    {// shadow execution
        auto configuration = std::make_unique<se::Configuration>();
        configuration->_cycle_bound = 10;
        auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
        engine->run(*shadow_cfg, "../../test/benchmark/dissertation/augmentation/01/P");
    }
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeShadow, Augmentation_02) {
    std::shared_ptr<Cfg> shadow_cfg = getCfg("../../test/benchmark/dissertation/augmentation/02/S/S2.st");
    std::cout << shadow_cfg->toDot() << std::endl;
    {// shadow execution
        auto configuration = std::make_unique<se::Configuration>();
        configuration->_cycle_bound = 10;
        auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
        engine->run(*shadow_cfg, "../../test/benchmark/dissertation/augmentation/02/P");
    }

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeShadow, Scenario0_2) {
    std::shared_ptr<Cfg> shadow_cfg = getCfg("../../test/benchmark/dissertation/PPU/reconfiguration/Scenario0_2.st");
    std::cout << shadow_cfg->toDot() << std::endl;
    {// shadow execution
        auto configuration = std::make_unique<se::Configuration>();
        configuration->_cycle_bound = 20;
        auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
        engine->run(*shadow_cfg, "../../test/benchmark/dissertation/PPU/ST/test_suite_scenario_0");
    }
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSeShadow, Scenario2_3) {
    std::shared_ptr<Cfg> shadow_cfg = getCfg("../../test/benchmark/dissertation/PPU/reconfiguration/Scenario2_3.st");
    std::cout << shadow_cfg->toDot() << std::endl;
    {// shadow execution
        auto configuration = std::make_unique<se::Configuration>();
        configuration->_cycle_bound = 5;
        auto engine = std::make_unique<se::shadow::Engine>(std::move(configuration));
        engine->run(*shadow_cfg, "../../test/benchmark/dissertation/PPU/ST/test_suite_scenario_2");
    }
    ASSERT_EQ(0, 1);
}