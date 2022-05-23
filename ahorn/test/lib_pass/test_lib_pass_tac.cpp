#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "pass/tac/tac_pass.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibPassTac : public ::testing::Test {
public:
    TestLibPassTac() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/tac.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("Tac", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Tac");
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

TEST_F(TestLibPassTac, Consecutive_Assignments) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/tac/consecutive_assignments.st");
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibPassTac, Preserve_Operand_Order) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/tac/preserve_operand_order.st");
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibPassTac, Boolean_Assignments) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/tac/boolean_assignments.st");
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibPassTac, Parts_Of_Antivalent) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/tac/parts_of_antivalent.st");
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}