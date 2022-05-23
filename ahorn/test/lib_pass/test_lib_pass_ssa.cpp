#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "pass/basic_block_pass.h"
#include "pass/ssa/ssa_pass.h"
#include "pass/tac/tac_pass.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibPassSsa : public ::testing::Test {
public:
    TestLibPassSsa() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Tac");
        createLogger("Ssa");
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
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Tac");
        spdlog::drop("Ssa");
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

TEST_F(TestLibPassSsa, Wikipedia_Example) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/ssa/wikipedia_example.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<BasicBlockPass> basic_block_pass = std::make_unique<BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    /*
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    */
    std::unique_ptr<SsaPass> ssa_pass = std::make_unique<SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*bb_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibPassSsa, Parts_Of_Antivalent) {
    using namespace pass;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_pass/file/ssa/parts_of_antivalent.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<BasicBlockPass> basic_block_pass = std::make_unique<BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<TacPass> tac_pass = std::make_unique<TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<SsaPass> ssa_pass = std::make_unique<SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    ASSERT_EQ(0, 1);
}