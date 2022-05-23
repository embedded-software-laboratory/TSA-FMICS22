#include "ir/instruction/assignment_instruction.h"

#include <sstream>

using namespace ir;

AssignmentInstruction::AssignmentInstruction(std::unique_ptr<VariableReference> variable_reference,
                                             std::unique_ptr<Expression> expression, Type type)
    : Instruction(Kind::ASSIGNMENT), _variable_reference(std::move(variable_reference)),
      _expression(std::move(expression)), _type(type) {}

std::ostream &AssignmentInstruction::print(std::ostream &os) const {
    std::stringstream str;
    str << *_variable_reference << " := " << *_expression << ";";
    return os << str.str();
}

const Expression &AssignmentInstruction::getExpression() const {
    return *_expression;
}

const VariableReference &AssignmentInstruction::getVariableReference() const {
    return *_variable_reference;
}

void AssignmentInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

AssignmentInstruction *AssignmentInstruction::clone_implementation() const {
    auto variable_reference = _variable_reference->clone();
    auto expression = _expression->clone();
    return new AssignmentInstruction(std::move(variable_reference), std::move(expression), getType());
}

void AssignmentInstruction::setType(AssignmentInstruction::Type type) {
    _type = type;
}

AssignmentInstruction::Type AssignmentInstruction::getType() const {
    return _type;
}
