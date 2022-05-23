#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "pass/basic_block_pass.h"
#include "pass/call_transformation_pass.h"
#include "pass/ssa/ssa_pass.h"
#include "pass/tac/tac_pass.h"
#include "sa/analyzer.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibSa : public ::testing::Test {
public:
    TestLibSa() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Tac");
        createLogger("Static Analysis");
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
        spdlog::drop("Static Analysis");
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

TEST_F(TestLibSa, Field_Access_Of_Callee) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_sa/file/field_access_of_callee.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::CallTransformationPass> ct_pass = std::make_unique<pass::CallTransformationPass>();
    std::shared_ptr<Cfg> ct_cfg = ct_pass->apply(*tac_cfg);
    std::cout << ct_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*ct_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, Modified_Antivalent) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_sa/file/modified_antivalent.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::CallTransformationPass> ct_pass = std::make_unique<pass::CallTransformationPass>();
    std::shared_ptr<Cfg> ct_cfg = ct_pass->apply(*tac_cfg);
    std::cout << ct_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*ct_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, FlattenedAntivalent) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("/home/magroc/Documents/GitLab/ahorn/test/benchmark/dissertation/PLCopen_safety"
                                      "/SA/FlattenedSFAntivalent_Test.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}


TEST_F(TestLibSa, Crab_Test) {
    std::vector<cfg_ref_t> cfgs;
    variable_factory_t crab_variable_factory;
    // main
    var_t x(crab_variable_factory["x"], crab::INT_TYPE, 32);
    var_t x_res(crab_variable_factory["x_res"], crab::INT_TYPE, 32);
    var_t cycle(crab_variable_factory["cycle"], crab::INT_TYPE, 32);
    interface_declaration_t interface_declaration("main", {}, {});
    auto *main_cfg = new cfg_t("init", "main_exit", interface_declaration);
    {
        // create basic blocks for cycle semantics
        basic_block_t &init = main_cfg->insert("init");
        basic_block_t &main_entry = main_cfg->insert("main_entry");
        basic_block_t &cycle_entry = main_cfg->insert("cycle_entry");
        basic_block_t &main_body = main_cfg->insert("main_body");
        basic_block_t &cycle_exit = main_cfg->insert("cycle_exit");
        basic_block_t &main_exit = main_cfg->insert("main_exit");
        // create control-flow for cycle semantics
        init.add_succ(main_entry);
        main_entry.add_succ(cycle_entry);
        cycle_entry.add_succ(main_body);
        main_body.add_succ(cycle_exit);
        cycle_exit.add_succ(main_entry);
        cycle_exit.add_succ(main_exit);
        // adding statements
        init.assign(cycle, 0);
        init.assign(x, 0);
        main_entry.assume(cycle <= 100);
        cycle_entry.add(cycle, cycle, 1);
        main_body.callsite("increment", {x_res, cycle}, {x, cycle});
        cycle_exit.assign(x, x_res);
        main_exit.assume(cycle > 100);
    }
    crab::outs() << *main_cfg;
    cfgs.push_back(*main_cfg);

    // callee
    var_t x_in(crab_variable_factory["x_in"], crab::INT_TYPE, 32);
    var_t x_out(crab_variable_factory["x_out"], crab::INT_TYPE, 32);
    var_t cycle_in(crab_variable_factory["cycle_in"], crab::INT_TYPE, 32);
    var_t cycle_out(crab_variable_factory["cycle_out"], crab::INT_TYPE, 32);
    interface_declaration_t callee_interface_declaration("increment", {x_in, cycle_in}, {x_out, cycle_out});
    cfg_t *callee_cfg = new cfg_t("entry", "exit", callee_interface_declaration);
    {
        // create basic blocks for cycle semantics
        basic_block_t &entry = callee_cfg->insert("entry");
        basic_block_t &body = callee_cfg->insert("body");
        basic_block_t &exit = callee_cfg->insert("exit");
        // create control-flow for cycle semantics
        entry.add_succ(body);
        body.add_succ(exit);
        // adding statements
        body.add(x_out, x_in, 1);
        exit.assign(cycle_out, cycle_in);
    }
    crab::outs() << *callee_cfg;
    cfgs.push_back(*callee_cfg);

    std::vector<std::string> unreachable_labels;
    callgraph_t cg(cfgs);
    crab::outs() << "Callgraph=\n" << cg << "\n";
    boxes_domain_t boxes_domain;
    crab::analyzer::inter_analyzer_parameters<callgraph_t> params;
    params.analyze_recursive_functions = true;
    top_down_interprocedural_analyzer_t interprocedural_analyzer(cg, boxes_domain, params);
    interprocedural_analyzer.run(boxes_domain);
    for (auto &v : boost::make_iterator_range(cg.nodes())) {
        auto callee_cfg = v.get_cfg();
        auto fdecl = callee_cfg.get_func_decl();
        crab::outs() << fdecl << "\n";
        for (auto &bb : callee_cfg) {
            basic_block_label_t basic_block_label = bb.label();
            auto invariant = interprocedural_analyzer.get_post(callee_cfg, basic_block_label);
            crab::outs() << crab::basic_block_traits<basic_block_t>::to_string(bb.label()) << "=" << invariant << "\n";
            if (invariant.is_bottom()) {
                unreachable_labels.push_back(basic_block_label);
            }
        }
        crab::outs() << "=================================\n";
    }
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSa, Unreachable_Branches_Of_Antivalent) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_sa/file/unreachable_branches_of_antivalent.st");
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, Unreachable_Branch_In_Call) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_sa/file/unreachable_branch_in_call.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::CallTransformationPass> ct_pass = std::make_unique<pass::CallTransformationPass>();
    std::shared_ptr<Cfg> ct_cfg = ct_pass->apply(*tac_cfg);
    std::cout << ct_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*ct_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, Unreachable_Branch_Realizable_Path) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/lib_sa/file/unreachable_branch_realizable_path.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::CallTransformationPass> ct_pass = std::make_unique<pass::CallTransformationPass>();
    std::shared_ptr<Cfg> ct_cfg = ct_pass->apply(*tac_cfg);
    std::cout << ct_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*ct_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, Benchmark_Antivalent_X) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/antivalentX.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, FlattenedSFEDM) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/dissertation/PLCopen_safety/SA/FlattenedSFEDM.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}

TEST_F(TestLibSa, scenario0v0_final) {
    using namespace sa;
    std::shared_ptr<Cfg> cfg = getCfg("../../test/benchmark/ppu/scenario0v0_final.st");
    std::cout << cfg->toDot() << std::endl;
    std::unique_ptr<Analyzer> analyzer = std::make_unique<Analyzer>();
    std::unique_ptr<pass::BasicBlockPass> basic_block_pass = std::make_unique<pass::BasicBlockPass>();
    std::shared_ptr<Cfg> bb_cfg = basic_block_pass->apply(*cfg);
    std::cout << bb_cfg->toDot() << std::endl;
    std::unique_ptr<pass::TacPass> tac_pass = std::make_unique<pass::TacPass>();
    std::shared_ptr<Cfg> tac_cfg = tac_pass->apply(*bb_cfg);
    std::cout << tac_cfg->toDot() << std::endl;
    std::unique_ptr<pass::SsaPass> ssa_pass = std::make_unique<pass::SsaPass>();
    std::shared_ptr<Cfg> ssa_cfg = ssa_pass->apply(*tac_cfg);
    std::cout << ssa_cfg->toDot() << std::endl;
    std::vector<std::string> unreachable_labels = analyzer->performValueSetAnalysis(*ssa_cfg);
    std::cout << "Unreachable labels:" << std::endl;
    for (const auto &unreachable_label : unreachable_labels) {
        std::cout << "label: " << unreachable_label << std::endl;
    }
}