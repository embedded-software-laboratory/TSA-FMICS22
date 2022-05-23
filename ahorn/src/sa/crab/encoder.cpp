#include "sa/crab/encoder.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/phi.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "sa/crab/marshaller.h"

using namespace sa;

Encoder::Encoder(Marshaller &marshaller) : _marshaller(&marshaller) {}

void Encoder::encode(const ir::Expression &expression) {
    expression.accept(*this);
}

void Encoder::visit(const ir::BinaryExpression &expression) {
    const ir::Expression &left_operand = expression.getLeftOperand();
    const ir::Expression &right_operand = expression.getRightOperand();
    switch (left_operand.getKind()) {
        case ir::Expression::Kind::VARIABLE_ACCESS: {
            auto *left_variable_access = dynamic_cast<const ir::VariableAccess *>(&left_operand);
            std::string left_operand_name = left_variable_access->getName();
            var_t left_operand_variable = _marshaller->getCrabVariable(left_operand_name);
            switch (right_operand.getKind()) {
                case ir::Expression::Kind::VARIABLE_ACCESS: {
                    auto *right_variable_access = dynamic_cast<const ir::VariableAccess *>(&right_operand);
                    std::string right_operand_name = right_variable_access->getName();
                    var_t right_operand_variable = _marshaller->getCrabVariable(right_operand_name);
                    switch (_marshaller->_instruction->getKind()) {
                        case ir::Instruction::Kind::ASSIGNMENT: {
                            auto *assignment_instruction =
                                    dynamic_cast<const ir::AssignmentInstruction *>(_marshaller->_instruction);
                            const ir::VariableReference &variable_reference =
                                    assignment_instruction->getVariableReference();
                            std::string variable_name = variable_reference.getName();
                            var_t variable = _marshaller->getCrabVariable(variable_name);
                            basic_block_t &bb = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                                 std::to_string(_marshaller->_label));
                            switch (expression.getBinaryOperator()) {
                                case ir::BinaryExpression::BinaryOperator::EQUALITY: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        bb.bool_assign(variable,
                                                       lin_exp_t(left_operand_variable) == right_operand_variable);
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        throw std::logic_error("Not implemented yet.");
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        bb.bool_assign(variable,
                                                       lin_exp_t(left_operand_variable) >= right_operand_variable);
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        throw std::logic_error("Not implemented yet.");
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::ADD: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        throw std::logic_error("Not implemented yet.");
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        bb.assign(variable, left_operand_variable + right_operand_variable);
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::LESS_THAN: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        bb.bool_assign(variable,
                                                       lin_exp_t(left_operand_variable) < right_operand_variable);
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        throw std::logic_error("Not implemented yet.");
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::BOOLEAN_AND: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        bb.bool_and(variable, left_operand_variable, right_operand_variable);
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        throw std::logic_error("Not implemented yet.");
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::BOOLEAN_OR: {
                                    if (variable.get_type() ==
                                        crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                                        bb.bool_or(variable, left_operand_variable, right_operand_variable);
                                    } else {
                                        assert(variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               left_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                                               right_operand_variable.get_type() ==
                                                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                                        throw std::logic_error("Not implemented yet.");
                                    }
                                    break;
                                }
                                case ir::BinaryExpression::BinaryOperator::EXPONENTIATION:
                                case ir::BinaryExpression::BinaryOperator::MULTIPLY:
                                case ir::BinaryExpression::BinaryOperator::DIVIDE:
                                case ir::BinaryExpression::BinaryOperator::MODULO:
                                case ir::BinaryExpression::BinaryOperator::SUBTRACT:
                                case ir::BinaryExpression::BinaryOperator::GREATER_THAN:
                                case ir::BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO:
                                case ir::BinaryExpression::BinaryOperator::INEQUALITY:
                                case ir::BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
                                    throw std::logic_error("Not implemented yet.");
                                default:
                                    throw std::runtime_error("Unexpected binary operator encountered.");
                            }
                            break;
                        }
                        case ir::Instruction::Kind::HAVOC:
                        case ir::Instruction::Kind::CALL:
                        case ir::Instruction::Kind::GOTO:
                        case ir::Instruction::Kind::IF:
                        case ir::Instruction::Kind::SEQUENCE:
                        case ir::Instruction::Kind::WHILE:
                            throw std::logic_error("Not implemented yet.");
                        default:
                            throw std::runtime_error("Unexpected instruction kind encountered.");
                    }
                    break;
                }
                case ir::Expression::Kind::BINARY_EXPRESSION:
                case ir::Expression::Kind::BOOLEAN_CONSTANT:
                case ir::Expression::Kind::INTEGER_CONSTANT:
                case ir::Expression::Kind::ENUMERATED_VALUE_CONSTANT:
                case ir::Expression::Kind::TIME_CONSTANT:
                case ir::Expression::Kind::NONDETERMINISTIC_CONSTANT:
                case ir::Expression::Kind::FIELD_ACCESS:
                case ir::Expression::Kind::UNARY_EXPRESSION:
                case ir::Expression::Kind::UNDEFINED:
                case ir::Expression::Kind::PHI:
                case ir::Expression::Kind::CHANGE:
                case ir::Expression::Kind::BOOLEAN_TO_INTEGER_CAST:
                case ir::Expression::Kind::INTEGER_TO_BOOLEAN_CAST:
                    throw std::logic_error("Not implemented yet.");
                default:
                    throw std::runtime_error("Unexpected expression kind encountered.");
            }
            break;
        }
        case ir::Expression::Kind::BOOLEAN_CONSTANT:
        case ir::Expression::Kind::INTEGER_CONSTANT:
        case ir::Expression::Kind::BINARY_EXPRESSION:
        case ir::Expression::Kind::ENUMERATED_VALUE_CONSTANT:
        case ir::Expression::Kind::TIME_CONSTANT:
        case ir::Expression::Kind::NONDETERMINISTIC_CONSTANT:
        case ir::Expression::Kind::FIELD_ACCESS:
        case ir::Expression::Kind::UNARY_EXPRESSION:
        case ir::Expression::Kind::UNDEFINED:
        case ir::Expression::Kind::PHI:
        case ir::Expression::Kind::CHANGE:
        case ir::Expression::Kind::BOOLEAN_TO_INTEGER_CAST:
        case ir::Expression::Kind::INTEGER_TO_BOOLEAN_CAST:
            throw std::logic_error("Not implemented yet.");
        default:
            throw std::runtime_error("Unexpected expression kind encountered.");
    }
}

void Encoder::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::ChangeExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::UnaryExpression &expression) {
    switch (expression.getUnaryOperator()) {
        case ir::UnaryExpression::UnaryOperator::NEGATION:
        case ir::UnaryExpression::UnaryOperator::UNARY_PLUS:
            throw std::logic_error("Not implemented yet.");
        case ir::UnaryExpression::UnaryOperator::COMPLEMENT: {
            ir::Expression &operand = expression.getOperand();
            switch (operand.getKind()) {
                case ir::Expression::Kind::VARIABLE_ACCESS: {
                    auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&operand);
                    assert(variable_access != nullptr);
                    std::string operand_name = variable_access->getName();
                    var_t operand_variable = _marshaller->getCrabVariable(operand_name);
                    switch (_marshaller->_instruction->getKind()) {
                        case ir::Instruction::Kind::ASSIGNMENT: {
                            auto *assignment_instruction =
                                    dynamic_cast<const ir::AssignmentInstruction *>(_marshaller->_instruction);
                            const ir::VariableReference &variable_reference =
                                    assignment_instruction->getVariableReference();
                            std::string variable_name = variable_reference.getName();
                            var_t variable = _marshaller->getCrabVariable(variable_name);
                            basic_block_t &bb = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                                 std::to_string(_marshaller->_label));
                            bb.bool_not_assign(variable, operand_variable);
                            break;
                        }
                        case ir::Instruction::Kind::IF: {
                            basic_block_t &bb_tt = _marshaller->_crab_cfg->get_node(
                                    Marshaller::BASIC_BLOCK_NAME_PREFIX + std::to_string(_marshaller->_label) + "_tt");
                            bb_tt.bool_not_assume(operand_variable);
                            basic_block_t &bb_ff = _marshaller->_crab_cfg->get_node(
                                    Marshaller::BASIC_BLOCK_NAME_PREFIX + std::to_string(_marshaller->_label) + "_ff");
                            bb_ff.bool_assume(operand_variable);
                            break;
                        }
                        case ir::Instruction::Kind::HAVOC:
                        case ir::Instruction::Kind::CALL:
                        case ir::Instruction::Kind::GOTO:
                        case ir::Instruction::Kind::SEQUENCE:
                        case ir::Instruction::Kind::WHILE:
                            throw std::logic_error("Not implemented yet.");
                        default:
                            throw std::runtime_error("Unexpected instruction kind encountered.");
                    }
                    break;
                }
                case ir::Expression::Kind::UNARY_EXPRESSION:
                case ir::Expression::Kind::BINARY_EXPRESSION:
                case ir::Expression::Kind::BOOLEAN_CONSTANT:
                case ir::Expression::Kind::INTEGER_CONSTANT:
                case ir::Expression::Kind::ENUMERATED_VALUE_CONSTANT:
                case ir::Expression::Kind::TIME_CONSTANT:
                case ir::Expression::Kind::NONDETERMINISTIC_CONSTANT:
                case ir::Expression::Kind::FIELD_ACCESS:
                case ir::Expression::Kind::UNDEFINED:
                case ir::Expression::Kind::PHI:
                case ir::Expression::Kind::CHANGE:
                case ir::Expression::Kind::BOOLEAN_TO_INTEGER_CAST:
                case ir::Expression::Kind::INTEGER_TO_BOOLEAN_CAST:
                default:
                    throw std::runtime_error("Unsupported operand kind encountered.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected unary operator encountered.");
    }
}

void Encoder::visit(const ir::BooleanConstant &expression) {
    switch (_marshaller->_instruction->getKind()) {
        case ir::Instruction::Kind::ASSIGNMENT: {
            auto *assignment_instruction = dynamic_cast<const ir::AssignmentInstruction *>(_marshaller->_instruction);
            const ir::VariableReference &variable_reference = assignment_instruction->getVariableReference();
            std::string variable_name = variable_reference.getName();
            var_t variable = _marshaller->getCrabVariable(variable_name);
            basic_block_t &bb = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                 std::to_string(_marshaller->_label));
            assert(variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE));
            bool value = expression.getValue();
            bb.bool_assign(variable, value ? lin_cst_t::get_true() : lin_cst_t::get_false());
            break;
        }
        case ir::Instruction::Kind::IF:
        case ir::Instruction::Kind::HAVOC:
        case ir::Instruction::Kind::CALL:
        case ir::Instruction::Kind::GOTO:
        case ir::Instruction::Kind::SEQUENCE:
        case ir::Instruction::Kind::WHILE:
            throw std::logic_error("Not implemented yet.");
        default:
            throw std::runtime_error("Unexpected instruction kind encountered.");
    }
}

void Encoder::visit(const ir::IntegerConstant &expression) {
    switch (_marshaller->_instruction->getKind()) {
        case ir::Instruction::Kind::ASSIGNMENT: {
            auto *assignment_instruction = dynamic_cast<const ir::AssignmentInstruction *>(_marshaller->_instruction);
            const ir::VariableReference &variable_reference = assignment_instruction->getVariableReference();
            std::string variable_name = variable_reference.getName();
            var_t variable = _marshaller->getCrabVariable(variable_name);
            basic_block_t &bb = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                 std::to_string(_marshaller->_label));
            assert(variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
            int value = expression.getValue();
            bb.assign(variable, value);
            break;
        }
        case ir::Instruction::Kind::IF:
        case ir::Instruction::Kind::HAVOC:
        case ir::Instruction::Kind::CALL:
        case ir::Instruction::Kind::GOTO:
        case ir::Instruction::Kind::SEQUENCE:
        case ir::Instruction::Kind::WHILE:
            throw std::logic_error("Not implemented yet.");
        default:
            throw std::runtime_error("Unexpected instruction kind encountered.");
    }
}

void Encoder::visit(const ir::TimeConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::EnumeratedValue &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::VariableAccess &expression) {
    std::string operand_name = expression.getName();
    var_t operand_variable = _marshaller->getCrabVariable(operand_name);
    switch (_marshaller->_instruction->getKind()) {
        case ir::Instruction::Kind::ASSIGNMENT: {
            auto *assignment_instruction = dynamic_cast<const ir::AssignmentInstruction *>(_marshaller->_instruction);
            const ir::VariableReference &variable_reference = assignment_instruction->getVariableReference();
            std::string variable_name = variable_reference.getName();
            var_t variable = _marshaller->getCrabVariable(variable_name);
            basic_block_t &bb = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                 std::to_string(_marshaller->_label));
            if (variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                assert(operand_variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE));
                bb.bool_assign(variable, operand_variable);
            } else {
                assert(variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16) &&
                       operand_variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                bb.assign(variable, operand_variable);
            }
            break;
        }
        case ir::Instruction::Kind::IF: {
            basic_block_t &bb_tt = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                    std::to_string(_marshaller->_label) + "_tt");
            bb_tt.bool_assume(operand_variable);
            basic_block_t &bb_ff = _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX +
                                                                    std::to_string(_marshaller->_label) + "_ff");
            bb_ff.bool_not_assume(operand_variable);
            break;
        }
        case ir::Instruction::Kind::HAVOC:
        case ir::Instruction::Kind::CALL:
        case ir::Instruction::Kind::GOTO:
        case ir::Instruction::Kind::SEQUENCE:
        case ir::Instruction::Kind::WHILE:
            throw std::logic_error("Not implemented yet.");
        default:
            throw std::runtime_error("Unexpected instruction kind encountered.");
    }
}

void Encoder::visit(const ir::FieldAccess &expression) {
    std::string operand_name = expression.getName() + "_output";
    var_t operand_variable = _marshaller->getCrabVariable(operand_name);
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Phi &expression) {
    std::shared_ptr<ir::Variable> variable = expression.getVariable();
    std::string name = variable->getName();
    var_t crab_variable = _marshaller->getCrabVariable(name);
    for (const auto &label_to_operand : expression.getLabelToOperand()) {
        unsigned int label = label_to_operand.first;
        if (label == _marshaller->_cfg->getEntryLabel() && _marshaller->_cfg->getType() == Cfg::Type::PROGRAM) {
            // TODO: Why was this removed?
            basic_block_t &bb = _marshaller->_crab_cfg->get_node("entry");
            const ir::VariableAccess &variable_access = *label_to_operand.second;
            std::string operand_name = variable_access.getName();
            var_t operand_variable = _marshaller->getCrabVariable(operand_name);
            if (crab_variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                bb.bool_assign(crab_variable, operand_variable);
            } else {
                assert(crab_variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                bb.assign(crab_variable, operand_variable);
            }
        } else {
            basic_block_t &bb =
                    _marshaller->_crab_cfg->get_node(Marshaller::BASIC_BLOCK_NAME_PREFIX + std::to_string(label));
            const ir::VariableAccess &variable_access = *label_to_operand.second;
            std::string operand_name = variable_access.getName();
            var_t operand_variable = _marshaller->getCrabVariable(operand_name);
            if (crab_variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                bb.bool_assign(crab_variable, operand_variable);
            } else {
                assert(crab_variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                bb.assign(crab_variable, operand_variable);
            }
        }
    }
}