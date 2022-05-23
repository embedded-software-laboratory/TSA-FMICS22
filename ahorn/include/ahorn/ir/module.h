#ifndef AHORN_MODULE_H
#define AHORN_MODULE_H

#include <gtest/gtest_prod.h>

#include "ir/instruction/instruction.h"
#include "ir/interface.h"
#include "ir/name.h"
#include "ir/variable.h"

#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TestLibIr_Module_Test;

namespace ir {
    /**
 * Representation of a translation unit, i.e., either a program, function block or function.
 */
    class Module : public Name {
    private:
        using variable_map_t = std::map<std::string, std::unique_ptr<Variable>>;
        using instruction_map_t = std::map<int, std::unique_ptr<Instruction>>;
        using module_ref_t = std::reference_wrapper<const Module>;
        using module_ref_map_t = std::map<std::string, module_ref_t>;

        FRIEND_TEST(::TestLibIr, Module);

    public:
        ~Module() override = default;

        enum class Kind { PROGRAM = 0, FUNCTION_BLOCK = 1, FUNCTION = 2 };

        Module(std::string name, Kind kind, std::unique_ptr<Interface> interface, module_ref_map_t name_to_module,
               instruction_map_t name_to_instruction, int entry_label, int exit_label);

        // disable copy and copy assignment
        Module(const Module &other) = delete;
        Module &operator=(const Module &) = delete;

    public:
        Kind getKind() const;
        Interface &getInterface() const;

        // expose labeled instructions
        instruction_map_t::const_iterator instructionsBegin() const;
        instruction_map_t::const_iterator instructionsEnd() const;

        int getEntryLabel() const;
        int getExitLabel() const;

        friend std::ostream &operator<<(std::ostream &os, Module const &module) {
            return module.print(os);
        }
        std::ostream &print(std::ostream &os) const;

    private:
        void updateFlattenedInterface();

    private:
        struct getVariableReference : std::unary_function<variable_map_t::value_type, Variable> {
            getVariableReference() = default;
            Variable &operator()(const variable_map_t::value_type &p) const {
                return *(p.second);
            }
        };

    private:
        using const_variable_it = boost::transform_iterator<getVariableReference, variable_map_t::const_iterator>;

    public:
        const_variable_it flattenedInterfaceBegin() const;
        const_variable_it flattenedInterfaceEnd() const;

    private:
        const Kind _kind;
        const std::unique_ptr<Interface> _interface;
        variable_map_t _name_to_flattened_variable;
        const module_ref_map_t _name_to_module;
        instruction_map_t _label_to_instruction;
        const int _entry_label;
        const int _exit_label;
    };
}// namespace ir

#endif//AHORN_MODULE_H
