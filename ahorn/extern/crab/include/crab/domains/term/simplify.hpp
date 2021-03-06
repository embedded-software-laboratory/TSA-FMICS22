#pragma once

#include <boost/optional.hpp>
#include <crab/domains/term/term_expr.hpp>
#include <crab/domains/term/term_operators.hpp>

/*
   Simplifiers for table terms after giving meaning to functors.
*/

namespace crab {
namespace domains {
namespace term {

// Common API to simplifiers
template <class Num, class Ftor> class Simplifier {
protected:
  using term_table_t = term_table<Num, Ftor>;
  using term_id_t = typename term_table_t::term_id_t;

  term_table_t &_ttbl;

public:
  Simplifier(term_table_t &term_table) : _ttbl(term_table) {}

  virtual ~Simplifier() {}

  virtual void simplify() = 0;

  // This should not modify the term table
  // FIXME: cannot make const the method without changing
  // constness of other methods.
  virtual boost::optional<term_id_t> simplify_term(term_id_t t) = 0;
};

// Trivial simplifier by giving standard mathematical meaning
// to arithmetic operators assuming that conmutativity,
// associativity etc properties hold as expected.
template <class Num> class NumSimplifier : Simplifier<Num, term_operator_t> {
  using simplifier_t = Simplifier<Num, term_operator_t>;

  using term_table_t = term_table<Num, term_operator_t>;
  using term_id_t = typename term_table_t::term_id_t;
  using term_t = typename term_table_t::term_t;

  // Simplify term f(left,right)
  boost::optional<term_id_t> simplify_term(term_operator_t f, term_id_t left,
                                           term_id_t right) {
    // Only consider these two rules:
    //   '/'('*'(x,y),x) = y
    //   '/'('*'(x,y),y) = x
    switch (f) {
    case TERM_OP_SDIV:
    case TERM_OP_UDIV: {
      term_t *tleft = this->_ttbl.get_term_ptr(left);
      term_t *tright = this->_ttbl.get_term_ptr(right);

      if ((tleft->kind() == TERM_APP) && term_ftor(tleft) == TERM_OP_MUL) {
        const std::vector<term_id_t> &args(term_args(tleft));
        assert(args.size() == 2);
        term_t *tl = this->_ttbl.get_term_ptr(args[0]);
        term_t *tr = this->_ttbl.get_term_ptr(args[1]);

        if (tl == tright)
          return args[1];
        if (tr == tright)
          return args[0];
      }
    }
    default:
      return boost::optional<term_id_t>();
    }
  }

public:
  NumSimplifier(term_table_t &term_table) : simplifier_t(term_table) {}

  void simplify() {}

  boost::optional<term_id_t> simplify_term(term_id_t t) {
    if (term_t *tt = this->_ttbl.get_term_ptr(t)) {
      if (tt->kind() == TERM_APP) {
        const std::vector<term_id_t> &args(term_args(tt));
        assert(args.size() == 2);
        return simplify_term(term_ftor(tt), args[0], args[1]);
      }
    }
    return boost::optional<term_id_t>();
  }
};

} // end namespace term
} // end namespace domains
} // end namespace crab
