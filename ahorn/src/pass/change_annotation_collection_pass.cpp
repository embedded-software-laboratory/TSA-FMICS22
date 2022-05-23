#include "pass/change_annotation_collection_pass.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/unary_expression.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"

using namespace pass;

std::set<unsigned int> ChangeAnnotationCollectionPass::apply(const Cfg &cfg) {
    std::set<unsigned int> change_annotated_labels = std::set<unsigned int>();

    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        for (unsigned int change_annotated_label : apply(*callee_it)) {
            change_annotated_labels.emplace(change_annotated_label);
        }
    }

    for (auto vertex_it = cfg.verticesBegin(); vertex_it != cfg.verticesEnd(); ++vertex_it) {
        unsigned int label = vertex_it->getLabel();
        if (label == cfg.getEntryLabel() || label == cfg.getExitLabel()) {
            continue;
        }
        const ir::Instruction *instruction = vertex_it->getInstruction();
        assert(instruction != nullptr);
        if (containsChangeExpression(*instruction)) {
            change_annotated_labels.emplace(label);
        }
    }

    return change_annotated_labels;
}

bool ChangeAnnotationCollectionPass::containsChangeExpression(const ir::Instruction &instruction) {
    _contains_change_expression = false;
    instruction.accept(*this);
    return _contains_change_expression;
}

void ChangeAnnotationCollectionPass::visit(const ir::AssignmentInstruction &instruction) {
    instruction.getExpression().accept(*this);
}

void ChangeAnnotationCollectionPass::visit(const ir::CallInstruction &instruction) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::IfInstruction &instruction) {
    instruction.getExpression().accept(*this);
}

void ChangeAnnotationCollectionPass::visit(const ir::SequenceInstruction &instruction) {
    for (auto instruction_it = instruction.instructionsBegin(); instruction_it != instruction.instructionsEnd();
         ++instruction_it) {
        instruction_it->accept(*this);
    }
}

void ChangeAnnotationCollectionPass::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::HavocInstruction &instruction) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::BinaryExpression &expression) {
    expression.getRightOperand().accept(*this);
    expression.getLeftOperand().accept(*this);
}

void ChangeAnnotationCollectionPass::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::ChangeExpression &expression) {
    _contains_change_expression = true;
}

void ChangeAnnotationCollectionPass::visit(const ir::UnaryExpression &expression) {
    expression.getOperand().accept(*this);
}

void ChangeAnnotationCollectionPass::visit(const ir::BooleanConstant &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::IntegerConstant &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::TimeConstant &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::EnumeratedValue &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::VariableAccess &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::FieldAccess &expression) {
    // do nothing
}

void ChangeAnnotationCollectionPass::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void ChangeAnnotationCollectionPass::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}
