#ifndef AHORN_TAC_PASS_H
#define AHORN_TAC_PASS_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "pass/tac/encoder.h"

#include <map>
#include <memory>
#include <vector>

namespace pass {
    // XXX Three-Address Code is a pure structural code-improving transformation, i.e., this pass is completely context
    // XXX insensitive.
    class TacPass : private ir::InstructionVisitor {
    private:
        friend class Encoder;

    private:
        static const std::string TEMPORARY_VARIABLE_NAME_PREFIX;

    public:
        TacPass();

        std::shared_ptr<Cfg> apply(const Cfg &cfg);

    private:
        std::unique_ptr<ir::VariableAccess>
        introduceTemporaryVariableExpression(std::unique_ptr<ir::Expression> expression);

        void insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label);

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
        // XXX current label of the vertex
        unsigned int _label;
        // XXX keeps track of values for implicit SSA-form
        unsigned int _value;
        // XXX set of temporary variables
        std::vector<std::shared_ptr<ir::Variable>> _temporary_variables;
        // XXX maps a label to a list of instructions including the introduced temporary assignments
        std::map<unsigned int, std::vector<std::unique_ptr<ir::Instruction>>> _label_to_instructions;
        // XXX maps type-representative cfg names to the corresponding TAC'ed cfg
        std::map<std::string, std::shared_ptr<Cfg>> _type_representative_name_to_cfg;
    };
}// namespace pass

#endif//AHORN_TAC_PASS_H
