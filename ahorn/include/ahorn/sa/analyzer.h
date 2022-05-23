#ifndef AHORN_SA_ANALYZER_H
#define AHORN_SA_ANALYZER_H

#include "cfg/cfg.h"
#include "sa/crab/marshaller.h"

// crab intraprocedural forward analyzer
#include "crab/analysis/fwd_analyzer.hpp"

// crab interprocedural top-down analyzer
#include "crab/analysis/inter/top_down_inter_analyzer.hpp"

// crab call graph
#include "crab/cg/cg.hpp"

// crab abstract domain
#include "crab/domains/abstract_domain.hpp"
#include "crab/domains/boxes.hpp"
#include "crab/domains/elina_domains.hpp"
#include "crab/domains/interval.hpp"
#include "crab/domains/split_dbm.hpp"

using interval_domain_t = ikos::interval_domain<ikos::z_number, varname_t>;
using boxes_domain_t = crab::domains::boxes_domain<ikos::z_number, varname_t>;
using zones_domain_t = crab::domains::elina_domain<ikos::z_number, varname_t, crab::domains::ELINA_ZONES>;
using z_dbm_graph_t =
        crab::domains::DBM_impl::DefaultParams<ikos::z_number, crab::domains::DBM_impl::GraphRep::adapt_ss>;
using split_dbm_t = crab::domains::split_dbm_domain<ikos::z_number, varname_t, z_dbm_graph_t>;

using cfg_ref_t = crab::cfg::cfg_ref<cfg_t>;
using fwd_analyzer_t = crab::analyzer::intra_fwd_analyzer<cfg_ref_t, boxes_domain_t>;
using fwd_zones_analyzer_t = crab::analyzer::intra_fwd_analyzer<cfg_ref_t, zones_domain_t>;
using fwd_split_analyzer_t = crab::analyzer::intra_fwd_analyzer<cfg_ref_t, split_dbm_t>;

using callgraph_t = crab::cg::call_graph<cfg_ref_t>;
using top_down_interprocedural_analyzer_t = crab::analyzer::top_down_inter_analyzer<callgraph_t, boxes_domain_t>;
using top_down_interprocedural_interval_analyzer_t =
        crab::analyzer::top_down_inter_analyzer<callgraph_t, interval_domain_t>;
using top_down_interprocedural_zones_analyzer_t = crab::analyzer::top_down_inter_analyzer<callgraph_t, zones_domain_t>;
using top_down_interprocedural_split_analyzer_t = crab::analyzer::top_down_inter_analyzer<callgraph_t, split_dbm_t>;

#include <memory>

namespace sa {
    class Analyzer {
    public:
        Analyzer();

        std::vector<std::string> performValueSetAnalysis(const Cfg &cfg);

    private:
        std::unique_ptr<Marshaller> _marshaller;
    };
}// namespace sa

#endif//AHORN_SA_ANALYZER_H
