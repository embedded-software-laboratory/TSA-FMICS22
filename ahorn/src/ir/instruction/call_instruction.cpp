#include "ir/instruction/call_instruction.h"

#include <sstream>

using namespace ir;

CallInstruction::CallInstruction(std::unique_ptr<VariableAccess> variable_access,
                                 std::unique_ptr<GotoInstruction> goto_intraprocedural,
                                 std::unique_ptr<GotoInstruction> goto_interprocedural)
    : Instruction(Kind::CALL), _variable_access(std::move(variable_access)),
      _goto_intraprocedural(std::move(goto_intraprocedural)), _goto_interprocedural(std::move(goto_interprocedural)) {}

std::ostream &CallInstruction::print(std::ostream &os) const {
    std::stringstream str;
    str << "call " << *_variable_access << "();";
    return os << str.str();
}

void CallInstruction::accept(InstructionVisitor &visitor) const {
    visitor.visit(*this);
}

CallInstruction *CallInstruction::clone_implementation() const {
    const auto &variable_access = getVariableAccess();
    auto *call_instruction = new CallInstruction(variable_access.clone(), _goto_intraprocedural->clone(),
                                                 _goto_interprocedural->clone());
    for (const auto &label_of_pre_assignment : _labels_of_pre_assignments) {
        call_instruction->addLabelOfPreAssignment(label_of_pre_assignment);
    }
    for (const auto &label_of_post_assignment : _labels_of_post_assignments) {
        call_instruction->addLabelOfPostAssignment(label_of_post_assignment);
    }
    return call_instruction;
}

const VariableAccess &CallInstruction::getVariableAccess() const {
    return *_variable_access;
}

void CallInstruction::addLabelOfPreAssignment(int label) {
    _labels_of_pre_assignments.push_back(label);
}

void CallInstruction::addLabelOfPostAssignment(int label) {
    _labels_of_post_assignments.push_back(label);
}

std::vector<int> CallInstruction::getLabelsOfPreAssignments() const {
    return _labels_of_pre_assignments;
}

std::vector<int> CallInstruction::getLabelsOfPostAssignments() const {
    return _labels_of_post_assignments;
}

GotoInstruction &CallInstruction::getGotoIntraprocedural() const {
    return *_goto_intraprocedural;
}

GotoInstruction &CallInstruction::getGotoInterprocedural() const {
    return *_goto_interprocedural;
}

int CallInstruction::getIntraproceduralLabel() const {
    return _goto_intraprocedural->getLabel();
}

int CallInstruction::getInterproceduralLabel() const {
    return _goto_interprocedural->getLabel();
}
