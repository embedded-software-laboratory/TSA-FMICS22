#include "ir/instruction/if_instruction.h"

#include <sstream>

using namespace ir;

IfInstruction::IfInstruction(std::unique_ptr<Expression> expression, std::unique_ptr<GotoInstruction> goto_then,
                             std::unique_ptr<GotoInstruction> goto_else)
    : Instruction(Kind::IF), _expression(std::move(expression)), _goto_then(std::move(goto_then)),
      _goto_else(std::move(goto_else)) {}

void IfInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &IfInstruction::print(std::ostream &os) const {
    std::stringstream str;
    str << "if " << *_expression;
    str << " then " << *_goto_then;
    str << " else " << *_goto_else;
    return os << str.str();
}

IfInstruction *IfInstruction::clone_implementation() const {
    auto expression = _expression->clone();
    auto goto_then = _goto_then->clone();
    auto goto_else = _goto_else->clone();
    return new IfInstruction(std::move(expression), std::move(goto_then), std::move(goto_else));
}

Expression &IfInstruction::getExpression() const {
    return *_expression;
}

GotoInstruction &IfInstruction::getGotoThen() const {
    return *_goto_then;
}

GotoInstruction &IfInstruction::getGotoElse() const {
    return *_goto_else;
}

int IfInstruction::getThenLabel() const {
    return _goto_then->getLabel();
}

int IfInstruction::getElseLabel() const {
    return _goto_else->getLabel();
}
