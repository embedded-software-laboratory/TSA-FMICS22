#ifndef AHORN_MANAGER_H
#define AHORN_MANAGER_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "ir/expression/constant/constant.h"
#include "ir/type/data_type.h"
#include "se/experimental/context/context.h"
#include "se/experimental/expression/expression.h"

#include "boost/optional/optional_io.hpp"
#include "z3++.h"

#include <map>
#include <memory>
#include <random>

class TestLibSe_Manager_Test;
class TestLibSe_Encoder_Test;
class TestLibSe_Executor_Test;

namespace se {
    class Manager {
    private:
        FRIEND_TEST(::TestLibSe, Manager);
        FRIEND_TEST(::TestLibSe, Encoder);
        FRIEND_TEST(::TestLibSe, Executor);

    private:
        friend class Engine;

    public:
        Manager();
        // XXX copy constructor disabled
        Manager(const Manager &other) = delete;
        // XXX copy assignment disabled
        Manager &operator=(const Manager &) = delete;

        z3::context &getZ3Context() const;

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        z3::expr makeBooleanValue(bool value);

        z3::expr makeIntegerValue(int value);

        z3::expr makeDefaultValue(const ir::DataType &data_type);

        z3::expr makeRandomValue(const ir::DataType &data_type);

        z3::expr makeValue(const ir::Constant &constant);

        z3::expr makeBooleanConstant(const std::string &contextualized_name);

        z3::expr makeIntegerConstant(const std::string &contextualized_name);

        z3::expr makeConstant(const std::string &contextualized_name, const ir::DataType &data_type);

        std::pair<z3::check_result, boost::optional<z3::model>> check(const z3::expr_vector &z3_expressions);

        z3::expr substituteZ3Expression(z3::expr z3_expression, const z3::expr &z3_source_expression,
                                        const z3::expr &z3_destination_expression) const;

        bool containsShadowExpression(const Context &context, const z3::expr &z3_expression, bool concrete = true);

        std::vector<z3::expr> getUninterpretedConstantsFromZ3Expression(const z3::expr &expression) const;

    private:
        // https://github.com/Z3Prover/z3/blob/master/examples/c%2B%2B/example.cpp#L805
        void visit(std::vector<bool> &visited, std::vector<z3::expr> &uninterpreted_constants,
                   const z3::expr &expression) const;

        void initialize(const Cfg &cfg);

    private:
        std::unique_ptr<z3::context> _z3_context;

        // "Globally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;

        std::set<std::string> _whole_program_inputs;

    private:
        // RNG for concrete valuations of "whole-program" inputs and havoc instructions
        std::mt19937 _random_number_generator;
        std::uniform_int_distribution<int> _int_distribution;
        std::uniform_int_distribution<int> _bool_distribution;
        std::uniform_int_distribution<int> _time_distribution;
    };
}// namespace se

#endif//AHORN_MANAGER_H
