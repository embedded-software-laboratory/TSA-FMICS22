#ifndef AHORN_HAVOC_INSTRUCTION_H
#define AHORN_HAVOC_INSTRUCTION_H

#include "ir/expression/expression.h"
#include "ir/expression/variable_reference.h"
#include "ir/instruction/instruction.h"

namespace ir {
    class HavocInstruction : public Instruction {
    public:
        // XXX default constructor disabled
        HavocInstruction() = delete;
        // XXX copy constructor disabled
        HavocInstruction(const HavocInstruction &other) = delete;
        // XXX copy assignment disabled
        HavocInstruction &operator=(const HavocInstruction &) = delete;

        explicit HavocInstruction(std::unique_ptr<VariableReference> variable_reference);

        std::ostream &print(std::ostream &os) const override;

        void accept(InstructionVisitor &visitor) const override;

        std::unique_ptr<HavocInstruction> clone() const {
            return std::unique_ptr<HavocInstruction>(static_cast<HavocInstruction *>(this->clone_implementation()));
        }

        const VariableReference &getVariableReference() const;

    private:
        HavocInstruction *clone_implementation() const override;

    private:
        const std::unique_ptr<VariableReference> _variable_reference;
    };
}// namespace ir

#endif//AHORN_HAVOC_INSTRUCTION_H
