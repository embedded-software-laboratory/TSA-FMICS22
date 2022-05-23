#ifndef AHORN_INTERFACE_H
#define AHORN_INTERFACE_H

#include <gtest/gtest_prod.h>

#include "boost/iterator/filter_iterator.hpp"
#include "boost/iterator/transform_iterator.hpp"
#include "ir/expression/variable_access.h"
#include "ir/variable.h"
#include <map>
#include <memory>
#include <vector>

class Cfg;
class TestLibCfg_If_Test;
class TestLibCompiler_Compiler_Test;

namespace ir {
    class Interface {
        friend class ::Cfg;

    private:
        FRIEND_TEST(::TestLibCfg, If);
        FRIEND_TEST(::TestLibCompiler, Compiler);

    public:
        using variable_t = std::shared_ptr<Variable>;
        using variable_vector_t = std::vector<variable_t>;
        using variable_map_t = std::map<std::string, variable_t>;

    public:
        // XXX default constructor disabled
        Interface() = delete;
        // XXX copy constructor disabled
        Interface(const Interface &other) = delete;
        // XXX copy assignment disabled
        Interface &operator=(const Interface &) = delete;

        explicit Interface(variable_vector_t variables);

    private:
        struct getVariableReference : std::unary_function<variable_map_t::value_type, Variable> {
            getVariableReference() = default;
            Variable &operator()(const variable_map_t::value_type &p) const {
                return *(p.second);
            }
        };

        struct isInputVariable {
            bool operator()(const Variable &variable) {
                return variable.getStorageType() == Variable::StorageType::INPUT;
            }
        };

        struct isLocalVariable {
            bool operator()(const Variable &variable) {
                return variable.getStorageType() == Variable::StorageType::LOCAL;
            }
        };

        struct isOutputVariable {
            bool operator()(const Variable &variable) {
                return variable.getStorageType() == Variable::StorageType::OUTPUT;
            }
        };

        struct isTemporaryVariable {
            bool operator()(const Variable &variable) {
                return variable.getStorageType() == Variable::StorageType::TEMPORARY;
            }
        };

    private:
        using const_variable_it = boost::transform_iterator<getVariableReference, variable_map_t::const_iterator>;
        using const_input_variable_it = boost::filter_iterator<isInputVariable, const_variable_it>;
        using const_local_variable_it = boost::filter_iterator<isLocalVariable, const_variable_it>;
        using const_output_variable_it = boost::filter_iterator<isOutputVariable, const_variable_it>;
        using const_temporary_variable_it = boost::filter_iterator<isTemporaryVariable, const_variable_it>;

        // XXX intentionally hidden due to invalidation of flattened interface
        const Variable &addVariable(variable_t variable);
        // XXX intentionally hidden due to invalidation of flattened interface
        void removeVariable(const std::string &name);

    public:
        const_variable_it variablesBegin() const;
        const_variable_it variablesEnd() const;
        const_input_variable_it inputVariablesBegin() const;
        const_input_variable_it inputVariablesEnd() const;
        const_local_variable_it localVariablesBegin() const;
        const_local_variable_it localVariablesEnd() const;
        const_output_variable_it outputVariablesBegin() const;
        const_output_variable_it outputVariablesEnd() const;
        const_temporary_variable_it temporaryVariablesBegin() const;
        const_temporary_variable_it temporaryVariablesEnd() const;

        // retrieves a variable by name, returns throws an exception when variable does not exist in the interface
        variable_t getVariable(const std::string &name) const;

        friend std::ostream &operator<<(std::ostream &os, Interface const &interface) {
            return interface.print(os);
        }

        std::ostream &print(std::ostream &os) const;

    private:
        variable_map_t _name_to_variable;
    };
}// namespace ir

#endif//AHORN_INTERFACE_H
