#include "compiler/analyzer.h"
#include "compiler/statement/assignment_statement.h"
#include "compiler/statement/case_statement.h"
#include "compiler/statement/if_statement.h"
#include "compiler/statement/invocation_statement.h"
#include "compiler/statement/while_statement.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/boolean_to_integer_cast.h"
#include "ir/expression/change_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/integer_to_boolean_cast.h"
#include "ir/expression/variable_access.h"
#include "ir/type/elementary_type.h"
#include "ir/type/enumerated_type.h"
#include "ir/type/safety_type.h"

#include <cassert>
#include <search.h>

using namespace ir;

void Analyzer::analyze() {
    for (auto it = _project.pousBegin(); it != _project.pousEnd(); ++it) {
        analyze(*it);
    }
}

void Analyzer::analyze(Pou &pou) {
    _pou = &pou;
    for (auto it = _pou->getBody().statementListBegin(); it != _pou->getBody().statementListEnd(); ++it) {
        it->accept(*this);
    }
}

void Analyzer::visit(AssertStatement &statement) {
    throw std::logic_error("Not implemented yet.");
}

void Analyzer::visit(AssignmentStatement &statement) {
    statement.getExpression().accept(*this);
    auto &expression = statement.getExpression();
    auto type = expression.getType();
    const DataType *data_type = nullptr;
    if (const auto *variable_access = dynamic_cast<const VariableAccess *>(&statement.getVariableReference())) {
        data_type = &variable_access->getVariable()->getDataType();
    } else if (const auto *field_access = dynamic_cast<const FieldAccess *>(&statement.getVariableReference())) {
        const auto &field_variable_access = field_access->getVariableAccess();
        data_type = &field_variable_access.getVariable()->getDataType();
    } else {
        throw std::logic_error("Invalid lhs of assignment statement.");
    }
    if (const auto *elementary_type = dynamic_cast<const ElementaryType *>(data_type)) {
        switch (elementary_type->getType()) {
            case ElementaryType::Type::INT: {
                if (type == Expression::Type::BOOLEAN) {
                    if (expression.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                        expression.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                        expression.getKind() == Expression::Kind::FIELD_ACCESS) {
                        statement.setExpression(std::make_unique<BooleanToIntegerCast>(expression.clone()));
                    } else {
                        throw std::logic_error("RHS has invalid type and kind.");
                    }
                } else if (type == Expression::Type::ARITHMETIC) {
                    // correct type, do nothing
                } else {
                    throw std::logic_error("Invalid expression type.");
                }
                break;
            }
            case ElementaryType::Type::REAL: {
                throw std::logic_error("REAL not implemented yet.");
            }
            case ElementaryType::Type::BOOL: {
                if (type == Expression::Type::BOOLEAN) {
                    // correct type, do nothing
                } else if (type == Expression::Type::ARITHMETIC) {
                    if (expression.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                        expression.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                        expression.getKind() == Expression::Kind::FIELD_ACCESS) {
                        statement.setExpression(std::make_unique<IntegerToBooleanCast>(expression.clone()));
                    } else {
                        throw std::logic_error("RHS has invalid type and kind.");
                    }
                } else {
                    throw std::logic_error("Invalid expression type.");
                }
                break;
            }
            case ElementaryType::Type::TIME: {
                if (type == Expression::Type::BOOLEAN) {
                    throw std::logic_error("RHS has invalid type and kind.");
                } else if (type == Expression::Type::ARITHMETIC) {
                    // correct type, do nothing
                } else {
                    throw std::logic_error("Invalid expression type.");
                }
                break;
            }
            default:
                throw std::logic_error("Invalid elementary type.");
        }
    } else if (const auto *safety_type = dynamic_cast<const SafetyType *>(data_type)) {
        switch (safety_type->getType()) {
            case SafetyType::Kind::SAFEBOOL: {
                if (type == Expression::Type::BOOLEAN) {
                    // correct type, do nothing
                } else if (type == Expression::Type::ARITHMETIC) {
                    if (expression.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                        expression.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                        expression.getKind() == Expression::Kind::FIELD_ACCESS) {
                        statement.setExpression(std::make_unique<IntegerToBooleanCast>(expression.clone()));
                    } else {
                        throw std::logic_error("RHS has invalid type and kind.");
                    }
                } else {
                    throw std::logic_error("Invalid expression type.");
                }
                break;
            }
        }
    } else if (data_type->getKind() == DataType::Kind::ENUMERATED_TYPE) {
        assert(expression.getKind() == Expression::Kind::ENUMERATED_VALUE_CONSTANT);
    } else {
        throw std::logic_error("Invalid type for LHS of assignment statement.");
    }
}

void Analyzer::visit(AssumeStatement &statement) {
    throw std::logic_error("Not implemented yet.");
}

void Analyzer::visit(CaseStatement &statement) {
    // TODO: type checking for case statements
}

void Analyzer::visit(IfStatement &statement) {
    Expression &expression = statement.getExpression();
    expression.accept(*this);
}

void Analyzer::visit(InvocationStatement &statement) {
    for (auto it = statement.preAssignmentsBegin(); it != statement.preAssignmentsEnd(); ++it) {
        it->accept(*this);
    }
    for (auto it = statement.postAssignmentsBegin(); it != statement.postAssignmentsEnd(); ++it) {
        it->accept(*this);
    }
}

void Analyzer::visit(WhileStatement &statement) {
    Expression &expression = statement.getExpression();
    expression.accept(*this);
}

void Analyzer::visit(BinaryExpression &expression) {
    // XXX descend
    expression.getLeftOperand().accept(*this);
    expression.getRightOperand().accept(*this);

    auto &left_operand = expression.getLeftOperand();
    auto &right_operand = expression.getRightOperand();
    auto type = expression.getType();
    auto left_type = expression.getLeftOperand().getType();
    auto right_type = expression.getRightOperand().getType();
    assert(type != Expression::Type::UNDEFINED);
    assert(left_type != Expression::Type::UNDEFINED);
    assert(right_type != Expression::Type::UNDEFINED);

    switch (expression.getBinaryOperator()) {
            // arithmetic operators
        case BinaryExpression::BinaryOperator::DIVIDE:
        case BinaryExpression::BinaryOperator::MODULO:
        case BinaryExpression::BinaryOperator::EXPONENTIATION: {
            assert(type == Expression::Type::ARITHMETIC);
            if (type == left_type && type == right_type) {
                // expression is correctly typed, do nothing
            } else if (left_type == Expression::Type::BOOLEAN || right_type == Expression::Type::BOOLEAN) {
                throw std::logic_error("BinaryExpression has invalid type.");
            }
            break;
        }
        case BinaryExpression::BinaryOperator::MULTIPLY:
        case BinaryExpression::BinaryOperator::ADD:
        case BinaryExpression::BinaryOperator::SUBTRACT: {
            assert(type == Expression::Type::ARITHMETIC);
            if (type == left_type && type == right_type) {
                // expression is correctly typed, do nothing
            } else if (type == left_type && type != right_type) {
                if (right_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    right_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    right_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setRightOperand(std::make_unique<BooleanToIntegerCast>(right_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else if (type != left_type && type == right_type) {
                if (left_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    left_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    left_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<BooleanToIntegerCast>(left_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else {
                // both types are not arithmetic
                if (left_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    left_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    left_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<BooleanToIntegerCast>(left_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
                if (right_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    right_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    right_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setRightOperand(std::make_unique<BooleanToIntegerCast>(right_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            }
            break;
        }
            // relational operators
        case BinaryExpression::BinaryOperator::GREATER_THAN:
        case BinaryExpression::BinaryOperator::LESS_THAN:
        case BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO:
        case BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO: {
            assert(type == Expression::Type::BOOLEAN);
            if (left_type == Expression::Type::BOOLEAN && right_type == Expression::Type::ARITHMETIC) {
                if (left_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    left_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    left_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<IntegerToBooleanCast>(left_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else if (left_type == Expression::Type::ARITHMETIC && right_type == Expression::Type::BOOLEAN) {
                if (right_operand.getKind() == Expression::Kind::BOOLEAN_CONSTANT ||
                    right_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    right_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<IntegerToBooleanCast>(right_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else if (left_type == Expression::Type::BOOLEAN && right_type == Expression::Type::BOOLEAN) {
                throw std::logic_error("BinaryExpression has invalid type.");
            }
            break;
        }
        case BinaryExpression::BinaryOperator::EQUALITY:
        case BinaryExpression::BinaryOperator::INEQUALITY: {
            assert(type == Expression::Type::BOOLEAN);
            if (left_type == Expression::Type::BOOLEAN && right_type == Expression::Type::BOOLEAN) {
                // expression is correctly typed, do nothing
            } else if (left_type == Expression::Type::ARITHMETIC && right_type == Expression::Type::ARITHMETIC) {
                // expression is correctly typed, do nothing
            } else {
                throw std::logic_error("Not implemented yet.");
            }
            break;
        }
            // boolean operators
        case BinaryExpression::BinaryOperator::BOOLEAN_AND:
        case BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        case BinaryExpression::BinaryOperator::BOOLEAN_OR: {
            assert(type == Expression::Type::BOOLEAN);
            if (type == left_type && type == right_type) {
                // expression is correctly typed, do nothing
            } else if (type == left_type && type != right_type) {
                if (right_operand.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                    right_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    right_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setRightOperand(std::make_unique<IntegerToBooleanCast>(right_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else if (type != left_type && type == right_type) {
                if (left_operand.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                    left_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    left_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<IntegerToBooleanCast>(left_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            } else {
                if (left_operand.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                    left_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    left_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setLeftOperand(std::make_unique<IntegerToBooleanCast>(left_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
                if (right_operand.getKind() == Expression::Kind::INTEGER_CONSTANT ||
                    right_operand.getKind() == Expression::Kind::VARIABLE_ACCESS ||
                    right_operand.getKind() == Expression::Kind::FIELD_ACCESS) {
                    expression.setRightOperand(std::make_unique<IntegerToBooleanCast>(right_operand.clone()));
                } else {
                    throw std::logic_error("BinaryExpression has invalid type.");
                }
            }
            break;
        }
        default:
            throw std::logic_error("Invalid binary operator.");
    }
}

void Analyzer::visit(ChangeExpression &expression) {
    // XXX descend
    expression.getOldOperand().accept(*this);
    expression.getNewOperand().accept(*this);
    // we disallow type changes in change expressions
    assert(expression.getOldOperand().getType() == expression.getNewOperand().getType());
    expression.setType(expression.getNewOperand().getType());
}

void Analyzer::visit(UnaryExpression &expression) {}

void Analyzer::visit(BooleanConstant &expression) {
    assert(expression.getType() == Expression::Type::BOOLEAN);
}

void Analyzer::visit(IntegerConstant &expression) {
    assert(expression.getType() == Expression::Type::ARITHMETIC);
}

void Analyzer::visit(TimeConstant &expression) {}

void Analyzer::visit(EnumeratedValue &expression) {}

void Analyzer::visit(NondeterministicConstant &expression) {}

void Analyzer::visit(Undefined &expression) {}

void Analyzer::visit(VariableAccess &expression) {}

void Analyzer::visit(FieldAccess &expression) {
    std::shared_ptr<Variable> record_variable = expression.getRecordAccess().getVariable();
    auto pou_type_name = record_variable->getDataType().getFullyQualifiedName();
}

void Analyzer::visit(Phi &expression) {}

void Analyzer::visit(BooleanToIntegerCast &expression) {}

void Analyzer::visit(IntegerToBooleanCast &expression) {}
