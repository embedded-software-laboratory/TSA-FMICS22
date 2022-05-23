#pragma once

//#include <crab/cfg/basic_block_traits.hpp>
#include <crab/cfg/cfg_bgl.hpp> // needed by wto.hpp
#include <crab/domains/interval.hpp>
#include <crab/iterators/wto.hpp>
#include <crab/support/debug.hpp>
#include <crab/types/linear_constraints.hpp>

#include <boost/range/iterator_range.hpp>

#include <algorithm>
#include <climits>
#include <unordered_map>

namespace crab {

namespace iterators {

/**
    Class that represents a set of thresholds used by the widening operator
**/
template <typename Number> class thresholds {

private:
  /// XXX: internal representation of a threshold
  using bound_t = ikos::bound<Number>;

  std::vector<bound_t> m_thresholds;
  size_t m_size;

  template <class B1, class B2> static B2 convert_bounds_impl(B1 b1) {
    B2 b2(0); // some initial value it doesn't matter which one
    ikos::bounds_impl::convert_bounds(b1, b2);
    return b2;
  }

public:
  thresholds(size_t size = UINT_MAX) : m_size(size) {
    m_thresholds.push_back(bound_t::minus_infinity());
    m_thresholds.push_back(0);
// useful thresholds for wrapped domains
#if 0
	 m_thresholds.push_back(bound_t("-2147483648"));	 
	 m_thresholds.push_back(bound_t("-32768"));
	 m_thresholds.push_back(bound_t("-128"));
	 m_thresholds.push_back(bound_t("127"));
	 m_thresholds.push_back(bound_t("255"));
	 m_thresholds.push_back(bound_t("32767"));
	 m_thresholds.push_back(bound_t("65535"));	 
	 m_thresholds.push_back(bound_t("2147483647"));
#endif
    m_thresholds.push_back(bound_t::plus_infinity());
  }

  unsigned size(void) const { return m_thresholds.size(); }

  template <typename N> void add(const ikos::bound<N> &v1) {
    if (m_thresholds.size() < m_size) {
      bound_t v = convert_bounds_impl<ikos::bound<N>, bound_t>(v1);
      if (std::find(m_thresholds.begin(), m_thresholds.end(), v) ==
          m_thresholds.end()) {
        auto ub = std::upper_bound(m_thresholds.begin(), m_thresholds.end(), v);

        // don't add consecutive thresholds
        if (v > 0) {
          auto prev = ub;
          --prev;
          if (prev != m_thresholds.begin()) {
            if (*prev + 1 == v) {
              *prev = v;
              return;
            }
          }
        } else if (v < 0) {
          if (*ub - 1 == v) {
            *ub = v;
            return;
          }
        }

        m_thresholds.insert(ub, v);
      }
    }
  }

  template <typename N>
  ikos::bound<N> get_next(const ikos::bound<N> &v1) const {
    if (v1.is_plus_infinity())
      return v1;
    bound_t v = convert_bounds_impl<ikos::bound<N>, bound_t>(v1);
    bound_t t = m_thresholds[m_thresholds.size() - 1];
    auto ub = std::upper_bound(m_thresholds.begin(), m_thresholds.end(), v);
    if (ub != m_thresholds.end())
      t = *ub;
    return convert_bounds_impl<bound_t, ikos::bound<N>>(t);
  }

  template <typename N>
  ikos::bound<N> get_prev(const ikos::bound<N> &v1) const {
    if (v1.is_minus_infinity())
      return v1;
    bound_t v = convert_bounds_impl<ikos::bound<N>, bound_t>(v1);
    auto lb = std::lower_bound(m_thresholds.begin(), m_thresholds.end(), v);
    if (lb != m_thresholds.end()) {
      --lb;
      if (lb != m_thresholds.end()) {
        return convert_bounds_impl<bound_t, ikos::bound<N>>(*lb);
      }
    }
    return convert_bounds_impl<bound_t, ikos::bound<N>>(m_thresholds[0]);
  }

  void write(crab_os &o) const {
    o << "{";
    for (typename std::vector<bound_t>::const_iterator
             it = m_thresholds.begin(),
             et = m_thresholds.end();
         it != et;) {
      bound_t b(*it);
      b.write(o);
      ++it;
      if (it != m_thresholds.end())
        o << ",";
    }
    o << "}";
  }
};

template <typename Number>
inline crab_os &operator<<(crab_os &o, const thresholds<Number> &t) {
  t.write(o);
  return o;
}

/**
   Collect thresholds per wto cycle (i.e. loop)
**/
template <typename CFG>
class wto_thresholds : public ikos::wto_component_visitor<CFG> {

public:
  using basic_block_label_t = typename CFG::basic_block_label_t;
  using wto_vertex_t = ikos::wto_vertex<CFG>;
  using wto_cycle_t = ikos::wto_cycle<CFG>;
  using thresholds_t = crab::iterators::thresholds<typename CFG::number_t>;
  using thresholds_map_t =
      std::unordered_map<basic_block_label_t, thresholds_t>;

private:
  // the cfg
  CFG m_cfg;
  // maximum number of thresholds
  size_t m_max_size;
  // keep a set of thresholds per wto head
  thresholds_map_t m_head_to_thresholds;
  // the top of the stack is the current wto head
  std::vector<basic_block_label_t> m_stack;

  using basic_block_t = typename CFG::basic_block_t;
  using number_t = typename CFG::number_t;
  using varname_t = typename CFG::varname_t;
  using variable_t = typename CFG::variable_t;
  using linear_expression_t = ikos::linear_expression<number_t, varname_t>;
  using linear_constraint_t = ikos::linear_constraint<number_t, varname_t>;
  using bound_t = ikos::bound<number_t>;

  using assume_t =
      crab::cfg::assume_stmt<basic_block_label_t, number_t, varname_t>;
  // using select_t = crab::cfg::select_stmt<basic_block_label_t,
  // number_t,varname_t>; using assign_t =
  // crab::cfg::assignment<basic_block_label_t, number_t,varname_t> ;

  void extract_bounds(const linear_expression_t &e, bool is_strict,
                      std::vector<number_t> &lb_bounds,
                      std::vector<number_t> &ub_bounds) const {
    if (e.size() == 1) {
      auto const &kv = *(e.begin());
      number_t coeff = kv.first;
      number_t k = -e.constant();
      if (coeff > 0) {
        // e is c*var <= k and c > 0  <---> var <= k/coeff
        ub_bounds.push_back(!is_strict ? k / coeff : (k / coeff) - 1);
        return;
      } else if (coeff < 0) {
        // e is c*var <= k  and c < 0 <---> var >= k/coeff
        lb_bounds.push_back(!is_strict ? k / coeff : (k / coeff) + 1);
        return;
      }
    }
  }

  void get_thresholds(const basic_block_t &bb, thresholds_t &thresholds) const {

    std::vector<number_t> lb_bounds, ub_bounds;
    for (auto const &i : boost::make_iterator_range(bb.begin(), bb.end())) {
      if (i.is_assume()) {
        auto a = static_cast<const assume_t *>(&i);
        const linear_constraint_t &cst = a->constraint();
        if (cst.is_inequality() || cst.is_strict_inequality()) {
          extract_bounds(cst.expression(), cst.is_strict_inequality(),
                         lb_bounds, ub_bounds);
        }
      }
      // else if (i.is_select()) {
      //   auto s = static_cast<const select_t*>(&i);
      //   linear_constraint_t cst = s->cond();
      //   if (cst.is_inequality() || cst.is_strict_inequality()) {
      //     extract_bounds(cst.expression(), cst.is_strict_inequality(),
      // 		      lb_bounds, ub_bounds);
      //   }
      // }
    }

    // Assuming that the variable is incremented/decremented by
    // some constant k, then we want to adjust the threshold to
    // +/- k so that we have more chance to stabilize in one
    // iteration after widening has been applied.
    int k = 1;
    for (auto n : lb_bounds) {
      thresholds.add(bound_t(n - k));
    }

    for (auto n : ub_bounds) {
      thresholds.add(bound_t(n + k));
    }
  }

public:
  wto_thresholds(CFG cfg, size_t max_size) : m_cfg(cfg), m_max_size(max_size) {}

  virtual void visit(wto_vertex_t &vertex) override {
    if (m_stack.empty())
      return;

    const basic_block_label_t &head = m_stack.back();
    auto it = m_head_to_thresholds.find(head);
    if (it != m_head_to_thresholds.end()) {
      thresholds_t &thresholds = it->second;
      typename CFG::basic_block_t &bb = m_cfg.get_node(vertex.node());
      get_thresholds(bb, thresholds);
    } else {
      CRAB_ERROR("No head found while gathering thresholds");
    }
  }

  virtual void visit(wto_cycle_t &cycle) override {
    thresholds_t thresholds(m_max_size);
    typename CFG::basic_block_t &bb = m_cfg.get_node(cycle.head());
    get_thresholds(bb, thresholds);

#if 1
    // XXX: if we want to consider constants from loop
    // initializations
    for (auto pre : boost::make_iterator_range(bb.prev_blocks())) {
      if (pre != cycle.head()) {
        typename CFG::basic_block_t &pred_bb = m_cfg.get_node(pre);
        get_thresholds(pred_bb, thresholds);
      }
    }
#endif

    m_head_to_thresholds.insert(std::make_pair(cycle.head(), thresholds));
    m_stack.push_back(cycle.head());
    for (typename wto_cycle_t::iterator it = cycle.begin(); it != cycle.end();
         ++it) {
      it->accept(this);
    }
    m_stack.pop_back();
  }

  const thresholds_map_t &get_thresholds_map() const {
    return m_head_to_thresholds;
  }

  void write(crab::crab_os &o) const {
    for (auto &kv : m_head_to_thresholds) {
      o << crab::basic_block_traits<basic_block_t>::to_string(kv.first) << "="
        << kv.second << "\n";
    }
  }

}; // class wto_thresholds

template <typename CFG>
inline crab::crab_os &operator<<(crab::crab_os &o,
                                 const wto_thresholds<CFG> &t) {
  t.write(o);
  return o;
}

} // end namespace iterators
} // end namespace crab
