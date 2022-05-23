#include "ir/instruction/goto_instruction.h"

#include <sstream>

using namespace ir;

GotoInstruction::GotoInstruction(int label) : Instruction(Kind::GOTO), _label(label) {}

void GotoInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &GotoInstruction::print(std::ostream &os) const {
    std::stringstream str;
    str << "goto L" << std::to_string(getLabel()) << ";";
    return os << str.str();
}

GotoInstruction *GotoInstruction::clone_implementation() const {
    return new GotoInstruction(getLabel());
}

int GotoInstruction::getLabel() const {
    return _label;
}

void GotoInstruction::setLabel(int label) {
    _label = label;
}
