#include "pass/ssa/ssa_encoder.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/enumerated_value.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/type/elementary_type.h"
#include "pass/ssa/ssa_pass.h"

using namespace pass;

SsaEncoder::SsaEncoder(SsaPass &ssa_pass) : _ssa_pass(&ssa_pass) {}

std::unique_ptr<ir::Expression> SsaEncoder::encode(const ir::Expression &expression) {
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    std::unique_ptr<ir::Expression> encoded_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    return encoded_expression;
}

void SsaEncoder::visit(const ir::BinaryExpression &expression) {
    expression.getLeftOperand().accept(*this);
    std::unique_ptr<ir::Expression> left_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    expression.getRightOperand().accept(*this);
    std::unique_ptr<ir::Expression> right_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    _expression_stack.push(std::make_unique<ir::BinaryExpression>(
            expression.getBinaryOperator(), std::move(left_expression), std::move(right_expression)));
}

void SsaEncoder::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void SsaEncoder::visit(const ir::ChangeExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void SsaEncoder::visit(const ir::UnaryExpression &expression) {
    expression.getOperand().accept(*this);
    std::unique_ptr<ir::Expression> encoded_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    _expression_stack.push(
            std::make_unique<ir::UnaryExpression>(expression.getUnaryOperator(), std::move(encoded_expression)));
}

void SsaEncoder::visit(const ir::BooleanConstant &expression) {
    int value = _ssa_pass->_value++;
    std::string constant_variable_name = SsaPass::CONSTANT_VARIABLE_NAME_PREFIX + std::to_string(value);
    std::shared_ptr<ir::Variable> constant_variable = std::make_shared<ir::Variable>(
            constant_variable_name, std::make_unique<ir::ElementaryType>("BOOL"), ir::Variable::StorageType::TEMPORARY);
    _ssa_pass->_value_to_variable.emplace(value, constant_variable);

    // introduce constant variable expression for substitution
    std::unique_ptr<ir::VariableAccess> constant_variable_expression =
            std::make_unique<ir::VariableAccess>(constant_variable);

    // introduce constant assignment, where the lhs is the constant variable and the rhs is the integer constant
    std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
            std::make_unique<ir::AssignmentInstruction>(constant_variable_expression->clone(), expression.clone());

    _ssa_pass->insertInstructionAtLabel(std::move(assignment_instruction), _ssa_pass->_label);

    _ssa_pass->writeVariable(constant_variable_name, _ssa_pass->_label, value);

    _expression_stack.push(std::move(constant_variable_expression));
}

void SsaEncoder::visit(const ir::IntegerConstant &expression) {
    int value = _ssa_pass->_value++;
    std::string constant_variable_name = SsaPass::CONSTANT_VARIABLE_NAME_PREFIX + std::to_string(value);
    std::shared_ptr<ir::Variable> constant_variable = std::make_shared<ir::Variable>(
            constant_variable_name, std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::TEMPORARY);
    _ssa_pass->_value_to_variable.emplace(value, constant_variable);

    // introduce constant variable expression for substitution
    std::unique_ptr<ir::VariableAccess> constant_variable_expression =
            std::make_unique<ir::VariableAccess>(constant_variable);

    // introduce constant assignment, where the lhs is the constant variable and the rhs is the integer constant
    std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
            std::make_unique<ir::AssignmentInstruction>(constant_variable_expression->clone(), expression.clone());

    _ssa_pass->insertInstructionAtLabel(std::move(assignment_instruction), _ssa_pass->_label);

    _ssa_pass->writeVariable(constant_variable_name, _ssa_pass->_label, value);

    _expression_stack.push(std::move(constant_variable_expression));
}

void SsaEncoder::visit(const ir::TimeConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

// XXX handle the same way as integer constant
void SsaEncoder::visit(const ir::EnumeratedValue &expression) {
    int value = _ssa_pass->_value++;
    std::string constant_variable_name = SsaPass::CONSTANT_VARIABLE_NAME_PREFIX + std::to_string(value);
    std::shared_ptr<ir::Variable> constant_variable = std::make_shared<ir::Variable>(
            constant_variable_name, std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::TEMPORARY);
    _ssa_pass->_value_to_variable.emplace(value, constant_variable);

    // introduce constant variable expression for substitution
    std::unique_ptr<ir::VariableAccess> constant_variable_expression =
            std::make_unique<ir::VariableAccess>(constant_variable);

    // introduce constant assignment, where the lhs is the constant variable and the rhs is the integer constant
    std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
            std::make_unique<ir::AssignmentInstruction>(constant_variable_expression->clone(), expression.clone());

    _ssa_pass->insertInstructionAtLabel(std::move(assignment_instruction), _ssa_pass->_label);

    _ssa_pass->writeVariable(constant_variable_name, _ssa_pass->_label, value);

    _expression_stack.push(std::move(constant_variable_expression));
}

void SsaEncoder::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void SsaEncoder::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void SsaEncoder::visit(const ir::VariableAccess &expression) {
    std::string name = expression.getName();
    int value = _ssa_pass->readVariable(name, _ssa_pass->_label);
    _expression_stack.push(std::make_unique<ir::VariableAccess>(_ssa_pass->_value_to_variable.at(value)));
}

void SsaEncoder::visit(const ir::FieldAccess &expression) {
    // XXX hack
    std::string name = expression.getName();
    _expression_stack.push(expression.clone());
}

void SsaEncoder::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void SsaEncoder::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}
