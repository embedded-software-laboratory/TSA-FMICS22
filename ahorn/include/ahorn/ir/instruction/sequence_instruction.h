#ifndef AHORN_SEQUENCE_INSTRUCTION_H
#define AHORN_SEQUENCE_INSTRUCTION_H

#include "ir/instruction/instruction.h"

#include "boost/iterator/indirect_iterator.hpp"

#include <memory>
#include <vector>

namespace ir {
    class SequenceInstruction : public Instruction {
    public:
        // XXX default constructor disabled
        SequenceInstruction() = delete;
        // XXX copy constructor disabled
        SequenceInstruction(const SequenceInstruction &other) = delete;
        // XXX copy assignment disabled
        SequenceInstruction &operator=(const SequenceInstruction &) = delete;

        explicit SequenceInstruction(std::vector<std::unique_ptr<Instruction>> instructions);

        std::ostream &print(std::ostream &os) const override;

        void accept(InstructionVisitor &visitor) const override;

        std::unique_ptr<SequenceInstruction> clone() const {
            return std::unique_ptr<SequenceInstruction>(
                    static_cast<SequenceInstruction *>(this->clone_implementation()));
        }

        boost::indirect_iterator<std::vector<std::unique_ptr<Instruction>>::const_iterator> instructionsBegin() const;
        
        boost::indirect_iterator<std::vector<std::unique_ptr<Instruction>>::const_iterator> instructionsEnd() const;

        bool containsIfInstruction() const;

    private:
        SequenceInstruction *clone_implementation() const override;

    private:
        std::vector<std::unique_ptr<Instruction>> _instructions;
    };
}// namespace ir

#endif//AHORN_SEQUENCE_INSTRUCTION_H
