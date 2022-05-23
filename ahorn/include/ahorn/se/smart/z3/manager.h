#ifndef AHORN_SMART_MANAGER_H
#define AHORN_SMART_MANAGER_H

#include "cfg/cfg.h"

#include "boost/optional/optional_io.hpp"
#include "z3++.h"

#include <memory>
#include <random>

namespace se::smart {
    class Manager {
    private:
        friend class Engine;

    public:
        Manager();

        z3::context &getZ3Context() const;

        z3::expr makeBooleanValue(bool value);

        z3::expr makeIntegerValue(int value);

        z3::expr makeDefaultValue(const ir::DataType &data_type);

        z3::expr makeRandomValue(const ir::DataType &data_type);

        z3::expr makeValue(const ir::Constant &constant);

        z3::expr makeBooleanConstant(const std::string &contextualized_name);

        z3::expr makeIntegerConstant(const std::string &contextualized_name);

        z3::expr makeConstant(const std::string &contextualized_name, const ir::DataType &data_type);

        std::pair<z3::check_result, boost::optional<z3::model>> check(const z3::expr_vector &expressions);

    private:
        std::unique_ptr<z3::context> _z3_context;

    private:
        // RNG for concrete valuations of "whole-program" inputs and havoc instructions
        std::mt19937 _random_number_generator;
        std::uniform_int_distribution<int> _int_distribution;
        std::uniform_int_distribution<int> _bool_distribution;
        std::uniform_int_distribution<int> _time_distribution;
    };
}// namespace se::smart

#endif//AHORN_SMART_MANAGER_H
