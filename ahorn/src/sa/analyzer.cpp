#include "sa/analyzer.h"

using namespace sa;

Analyzer::Analyzer() : _marshaller(std::make_unique<Marshaller>()) {}

std::vector<std::string> Analyzer::performValueSetAnalysis(const Cfg &cfg) {
    cfg_t *crab_cfg = _marshaller->marshall(cfg);
    std::vector<std::string> unreachable_labels;
    // XXX check if either intra- or interprocedural analysis should be applied
    if (_marshaller->_type_representative_name_to_crab_cfg.size() == 1) {
        std::cout << "Marshalled crab cfg:" << std::endl;
        crab::outs() << *crab_cfg;
        boxes_domain_t boxes_domain;
        crab::domains::crab_domain_params_man::get().set_param("boxes.ldd_size", "32768");
        crab::domains::crab_domain_params_man::get().set_param("boxes.convexify_threshold", "32768");
        crab::domains::crab_domain_params_man::get().set_param("boxes.dynamic_reordering", "true");
        // split_dbm_t split_domain;
        unsigned widening_delay = 1;
        unsigned narrowing_iterations = 2;
        fwd_analyzer_t forward_analyzer(*crab_cfg, boxes_domain, nullptr, nullptr, widening_delay, narrowing_iterations,
                                        20);
        forward_analyzer.run();
        crab::outs() << "Invariants using intraprocedural analysis with split domain:\n";
        for (auto &bb : *crab_cfg) {
            basic_block_label_t basic_block_label = bb.label();
            auto invariant = forward_analyzer.get_post(basic_block_label);
            if (basic_block_label == "bb_61_ff") {
                crab::outs() << crab::basic_block_traits<basic_block_t>::to_string(bb.label()) << "=" << invariant
                             << "\n";
            }
            // crab::outs() << crab::basic_block_traits<basic_block_t>::to_string(bb.label()) << "=" << invariant <<
            // "\n";
            if (invariant.is_bottom()) {
                unreachable_labels.push_back(basic_block_label);
            }
        }
    } else {
        std::vector<cfg_ref_t> cfgs;
        for (const auto &type_representative_name_to_crab_cfg : _marshaller->_type_representative_name_to_crab_cfg) {
            crab::outs() << "Marshalled crab cfg:\n" << *type_representative_name_to_crab_cfg.second;
            cfgs.emplace_back(*type_representative_name_to_crab_cfg.second);
        }
        callgraph_t cg(cfgs);
        crab::outs() << "Callgraph=\n" << cg << "\n";
        boxes_domain_t boxes_domain;
        crab::domains::crab_domain_params_man::get().set_param("boxes.ldd_size", "8192");
        crab::domains::crab_domain_params_man::get().set_param("boxes.convexify_threshold", "4096");
        crab::domains::crab_domain_params_man::get().set_param("boxes.dynamic_reordering", "false");
        // split_dbm_t split_domain;
        crab::analyzer::inter_analyzer_parameters<callgraph_t> params;
        params.widening_delay = 0;
        params.max_call_contexts = 1;
        top_down_interprocedural_analyzer_t interprocedural_analyzer(cg, boxes_domain, params);
        interprocedural_analyzer.run(boxes_domain);
        for (auto &v : boost::make_iterator_range(cg.nodes())) {
            auto callee_cfg = v.get_cfg();
            auto fdecl = callee_cfg.get_func_decl();
            crab::outs() << fdecl << "\n";
            for (auto &bb : callee_cfg) {
                basic_block_label_t basic_block_label = bb.label();
                auto invariant = interprocedural_analyzer.get_post(callee_cfg, basic_block_label);
                if (basic_block_label == "bb_90_ff") {
                    crab::outs() << crab::basic_block_traits<basic_block_t>::to_string(bb.label()) << "=" << invariant
                                 << "\n";
                }
                if (invariant.is_bottom()) {
                    unreachable_labels.push_back(basic_block_label);
                }
            }
            crab::outs() << "=================================\n";
        }
    }
    return unreachable_labels;
}