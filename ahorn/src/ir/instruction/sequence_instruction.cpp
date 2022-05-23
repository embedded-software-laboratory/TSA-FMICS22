#include "ir/instruction/sequence_instruction.h"

#include <sstream>

using namespace ir;

SequenceInstruction::SequenceInstruction(std::vector<std::unique_ptr<Instruction>> instructions)
    : Instruction(Kind::SEQUENCE), _instructions(std::move(instructions)) {}

std::ostream &SequenceInstruction::print(std::ostream &os) const {
    std::stringstream str;
    for (auto it = _instructions.begin(); it != _instructions.end(); ++it) {
        str << **it;
        if (std::next(it) != _instructions.end()) {
            str << "\n";
        }
    }
    return os << str.str();
}

void SequenceInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Instruction>>::const_iterator>
SequenceInstruction::instructionsBegin() const {
    return std::begin(_instructions);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Instruction>>::const_iterator>
SequenceInstruction::instructionsEnd() const {
    return std::end(_instructions);
}

bool SequenceInstruction::containsIfInstruction() const {
    return _instructions.back()->getKind() == ir::Instruction::Kind::IF;
}

SequenceInstruction *SequenceInstruction::clone_implementation() const {
    std::vector<std::unique_ptr<Instruction>> instructions;
    for (const std::unique_ptr<ir::Instruction> &instruction : _instructions) {
        instructions.emplace_back(instruction->clone());
    }
    return new SequenceInstruction(std::move(instructions));
}