#include "ir/instruction/havoc_instruction.h"

using namespace ir;

HavocInstruction::HavocInstruction(std::unique_ptr<VariableReference> variable_reference)
    : Instruction(Kind::HAVOC), _variable_reference(std::move(variable_reference)) {}

std::ostream &HavocInstruction::print(std::ostream &os) const {
    return os << "Havoc(" << *_variable_reference << ");";
}

void HavocInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

HavocInstruction *HavocInstruction::clone_implementation() const {
    auto variable_reference = _variable_reference->clone();
    return new HavocInstruction(std::move(variable_reference));
}

const VariableReference &HavocInstruction::getVariableReference() const {
    return *_variable_reference;
}