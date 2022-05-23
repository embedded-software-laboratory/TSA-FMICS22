#ifndef AHORN_GOTO_INSTRUCTION_H
#define AHORN_GOTO_INSTRUCTION_H

#include "ir/instruction/instruction.h"

namespace ir {
    class GotoInstruction : public Instruction {
    public:
        explicit GotoInstruction(int label);
        void accept(InstructionVisitor &visitor) const override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<GotoInstruction> clone() const {
            return std::unique_ptr<GotoInstruction>(static_cast<GotoInstruction *>(this->clone_implementation()));
        }
        void setLabel(int label);
        int getLabel() const;

    private:
        GotoInstruction *clone_implementation() const override;

    private:
        int _label;
    };
}// namespace ir

#endif//AHORN_GOTO_INSTRUCTION_H
