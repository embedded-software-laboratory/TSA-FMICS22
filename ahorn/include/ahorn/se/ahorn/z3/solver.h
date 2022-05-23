#ifndef AHORN_AHORN_SOLVER_H
#define AHORN_AHORN_SOLVER_H

#include "ir/expression/constant/constant.h"
#include "ir/type/data_type.h"

#include "boost/optional.hpp"
#include "z3++.h"

#include <memory>
#include <random>
#include <vector>

namespace se::ahorn {
    class Solver {
    public:
        Solver();
        // XXX copy constructor disabled
        Solver(const Solver &other) = delete;
        // XXX copy assignment disabled
        Solver &operator=(const Solver &) = delete;

        z3::context &getContext();

        z3::expr makeBooleanValue(bool value);

        z3::expr makeIntegerValue(int value);

        z3::expr makeDefaultValue(const ir::DataType &data_type);

        z3::expr makeRandomValue(const ir::DataType &data_type);

        z3::expr makeValue(const ir::Constant &constant);

        z3::expr makeBooleanConstant(const std::string &contextualized_name);

        z3::expr makeIntegerConstant(const std::string &contextualized_name);

        z3::expr makeConstant(const std::string &contextualized_name, const ir::DataType &data_type);

        std::pair<z3::check_result, boost::optional<z3::model>> check(const z3::expr_vector &expressions);

        std::vector<z3::expr> getUninterpretedConstants(const z3::expr &expression) const;

    private:
        // https://github.com/Z3Prover/z3/blob/master/examples/c%2B%2B/example.cpp#L805
        void visit(std::vector<bool> &visited, std::vector<z3::expr> &uninterpreted_constants,
                   const z3::expr &expression) const;

    private:
        std::unique_ptr<z3::context> _context;

    private:
        // RNG for concrete valuations of "whole-program" inputs and havoc instructions
        std::mt19937 _random_number_generator;
        std::uniform_int_distribution<int> _int_distribution;
        std::uniform_int_distribution<int> _bool_distribution;
        std::uniform_int_distribution<int> _time_distribution;
    };
}// namespace se::ahorn

#endif//AHORN_AHORN_SOLVER_H
