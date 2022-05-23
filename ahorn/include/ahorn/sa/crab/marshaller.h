#ifndef AHORN_MARSHALLER_H
#define AHORN_MARSHALLER_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "sa/crab/encoder.h"

// crab cfg
#include "crab/cfg/cfg.hpp"
// crab variable factory
#include "crab/types/variable.hpp"
#include "crab/types/varname_factory.hpp"

using variable_factory_t = crab::var_factory_impl::str_variable_factory;
using varname_t = typename variable_factory_t::varname_t;
template<>
class crab::variable_name_traits<std::string> {
public:
    static std::string to_string(std::string varname) {
        return varname;
    }
};
using var_t = crab::variable<ikos::z_number, varname_t>;
using lin_exp_t = ikos::linear_expression<ikos::z_number, varname_t>;
using lin_cst_t = ikos::linear_constraint<ikos::z_number, varname_t>;

using basic_block_label_t = std::string;
using cfg_t = crab::cfg::cfg<basic_block_label_t, varname_t, ikos::z_number>;
using basic_block_t = cfg_t::basic_block_t;
template<>
class crab::basic_block_traits<basic_block_t> {
public:
    using bb_label_t = typename basic_block_t::basic_block_label_t;
    static std::string to_string(const bb_label_t &bbl) {
        return bbl;
    }
};

using interface_declaration_t = crab::cfg::function_decl<ikos::z_number, varname_t>;

#include <map>
#include <memory>

namespace sa {
    class Marshaller : private ir::InstructionVisitor {
    private:
        friend class Analyzer;
        friend class Encoder;

    private:
        static const std::string BASIC_BLOCK_NAME_PREFIX;

    public:
        Marshaller();

        cfg_t *marshall(const Cfg &cfg);

    private:
        var_t getCrabVariable(const std::string &name) const;

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        std::unique_ptr<Encoder> _encoder;
        variable_factory_t _crab_variable_factory;

        std::map<std::string, cfg_t *> _type_representative_name_to_crab_cfg;

        // maps an SSA-valued name to its corresponding crab variable
        std::map<std::string, var_t> _name_to_crab_variable;

        // XXX fields required by the encoder
        // keeps track of the current crab cfg
        cfg_t *_crab_cfg;
        // keeps track of the current label
        unsigned int _label;
        // keeps track of the current instruction
        const ir::Instruction *_instruction;

        const Cfg *_cfg;
    };
}// namespace sa

#endif//AHORN_MARSHALLER_H
