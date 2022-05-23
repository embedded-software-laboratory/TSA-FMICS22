#ifndef AHORN_CALL_TRANSFORMATION_PASS_H
#define AHORN_CALL_TRANSFORMATION_PASS_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction.h"
#include "ir/instruction/instruction_visitor.h"

#include <map>
#include <memory>
#include <vector>

namespace pass {
    class CallTransformationPass : private ir::InstructionVisitor {
    public:
        CallTransformationPass() = default;

        std::shared_ptr<Cfg> apply(const Cfg &cfg);

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        void insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label,
                                      bool in_front = false);

    private:
        const Cfg *_cfg;
        std::map<std::string, std::shared_ptr<Cfg>> _type_representative_name_to_cfg;
        unsigned int _label;
        std::map<unsigned int, std::vector<std::unique_ptr<ir::Instruction>>> _label_to_instructions;
    };
}// namespace pass

#endif//AHORN_CALL_TRANSFORMATION_PASS_H
