#include "ir/instruction/while_instruction.h"

#include <sstream>

using namespace ir;

WhileInstruction::WhileInstruction(std::unique_ptr<Expression> expression,
                                   std::unique_ptr<GotoInstruction> goto_body_entry,
                                   std::unique_ptr<GotoInstruction> goto_while_exit)
    : Instruction(Kind::WHILE), _expression(std::move(expression)), _goto_body_entry(std::move(goto_body_entry)),
      _goto_while_exit(std::move(goto_while_exit)) {}

void WhileInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &WhileInstruction::print(std::ostream &os) const {
    std::stringstream str;
    str << "while (" << *_expression << ")"
        << " do " << *_goto_body_entry << " else " << *_goto_while_exit;
    return os << str.str();
}

WhileInstruction *WhileInstruction::clone_implementation() const {
    auto expression = _expression->clone();
    auto goto_body_entry = _goto_body_entry->clone();
    auto goto_while_exit = _goto_while_exit->clone();
    return new WhileInstruction(std::move(expression), std::move(goto_body_entry), std::move(goto_while_exit));
}

Expression &WhileInstruction::getExpression() const {
    return *_expression;
}

int WhileInstruction::getBodyEntryLabel() const {
    return _goto_body_entry->getLabel();
}

int WhileInstruction::getWhileExitLabel() const {
    return _goto_while_exit->getLabel();
}

GotoInstruction &WhileInstruction::getGotoWhileExit() const {
    return *_goto_while_exit;
}
