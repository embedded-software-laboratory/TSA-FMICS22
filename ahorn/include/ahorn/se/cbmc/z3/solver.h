#ifndef AHORN_CBMC_SOLVER_H
#define AHORN_CBMC_SOLVER_H

#include "cfg/cfg.h"
#include "ir/expression/constant/constant.h"
#include "ir/type/data_type.h"

#include "boost/optional.hpp"

#include "z3++.h"

#include <map>
#include <memory>
#include <random>
#include <set>

namespace se::cbmc {
    class Solver {
    private:
        friend class Engine;
        
    public:
        Solver();

        z3::context &getContext();

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        z3::expr makeBooleanValue(bool value);

        z3::expr makeIntegerValue(int value);

        z3::expr makeDefaultValue(const ir::DataType &data_type);

        z3::expr makeRandomValue(const ir::DataType &data_type);

        z3::expr makeValue(const ir::Constant &constant);

        z3::expr makeBooleanConstant(const std::string &contextualized_name);

        z3::expr makeIntegerConstant(const std::string &contextualized_name);

        z3::expr makeConstant(const std::string &contextualized_name, const ir::DataType &data_type);

        std::pair<z3::check_result, boost::optional<z3::model>>
        checkUnderAssumptions(const z3::expr_vector &expressions, const z3::expr_vector &assumptions);

    private:
        void initialize(const Cfg &cfg);

    private:
        std::unique_ptr<z3::context> _context;

        // "Globally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;

    private:
        // RNG for concrete valuations of "whole-program" inputs and havoc instructions
        std::mt19937 _random_number_generator;
        std::uniform_int_distribution<int> _int_distribution;
        std::uniform_int_distribution<int> _bool_distribution;
        std::uniform_int_distribution<int> _time_distribution;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_SOLVER_H
