#include "pass/tac/encoder.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/enumerated_value.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/type/elementary_type.h"
#include "pass/ssa/ssa_encoder.h"
#include "pass/tac/tac_pass.h"

#include <cassert>
#include <stdexcept>

using namespace pass;

Encoder::Encoder(TacPass &tac_pass) : _tac_pass(&tac_pass) {}

std::unique_ptr<ir::Expression> Encoder::encode(const ir::Expression &expression) {
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    std::unique_ptr<ir::Expression> encoded_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    return encoded_expression;
}

void Encoder::visit(const ir::BinaryExpression &expression) {
    expression.getLeftOperand().accept(*this);
    std::unique_ptr<ir::Expression> left_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    bool introduce_temporary_for_left_expression = false;
    if (left_expression->getKind() == ir::Expression::Kind::BINARY_EXPRESSION ||
        left_expression->getKind() == ir::Expression::Kind::UNARY_EXPRESSION) {
        introduce_temporary_for_left_expression = true;
        // push the temporary variable of the lhs as an expression onto the expression stack in order to substitute
        // the left expression of this expression.
        _expression_stack.push(_tac_pass->introduceTemporaryVariableExpression(std::move(left_expression)));
    }
    expression.getRightOperand().accept(*this);
    std::unique_ptr<ir::Expression> right_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    bool introduce_temporary_for_right_expression = false;
    if (right_expression->getKind() == ir::Expression::Kind::BINARY_EXPRESSION ||
        right_expression->getKind() == ir::Expression::Kind::UNARY_EXPRESSION) {
        introduce_temporary_for_right_expression = true;
        _expression_stack.push(_tac_pass->introduceTemporaryVariableExpression(std::move(right_expression)));
    }

    if (introduce_temporary_for_left_expression && introduce_temporary_for_right_expression) {
        std::unique_ptr<ir::Expression> temporary_right_expression = std::move(_expression_stack.top());
        _expression_stack.pop();
        std::unique_ptr<ir::Expression> temporary_left_expression = std::move(_expression_stack.top());
        _expression_stack.pop();
        _expression_stack.push(std::make_unique<ir::BinaryExpression>(expression.getBinaryOperator(),
                                                                      std::move(temporary_left_expression),
                                                                      std::move(temporary_right_expression)));
    } else if (introduce_temporary_for_left_expression && !introduce_temporary_for_right_expression) {
        std::unique_ptr<ir::Expression> temporary_left_expression = std::move(_expression_stack.top());
        _expression_stack.pop();
        _expression_stack.push(std::make_unique<ir::BinaryExpression>(
                expression.getBinaryOperator(), std::move(temporary_left_expression), std::move(right_expression)));
    } else if (!introduce_temporary_for_left_expression && introduce_temporary_for_right_expression) {
        std::unique_ptr<ir::Expression> temporary_right_expression = std::move(_expression_stack.top());
        _expression_stack.pop();
        _expression_stack.push(std::make_unique<ir::BinaryExpression>(
                expression.getBinaryOperator(), std::move(left_expression), std::move(temporary_right_expression)));
    } else {
        assert(!introduce_temporary_for_left_expression && !introduce_temporary_for_right_expression);
        _expression_stack.push(expression.clone());
    }
}

void Encoder::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::ChangeExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::UnaryExpression &expression) {
    expression.getOperand().accept(*this);
    std::unique_ptr<ir::Expression> encoded_expression = std::move(_expression_stack.top());
    _expression_stack.pop();
    if (encoded_expression->getKind() == ir::Expression::Kind::BINARY_EXPRESSION) {
        std::unique_ptr<ir::Expression> temporary_expression =
                _tac_pass->introduceTemporaryVariableExpression(std::move(encoded_expression));
        _expression_stack.push(
                std::make_unique<ir::UnaryExpression>(expression.getUnaryOperator(), std::move(temporary_expression)));
    } else {
        _expression_stack.push(
                std::make_unique<ir::UnaryExpression>(expression.getUnaryOperator(), std::move(encoded_expression)));
    }
}

void Encoder::visit(const ir::BooleanConstant &expression) {
    _expression_stack.emplace(expression.clone());
}

void Encoder::visit(const ir::IntegerConstant &expression) {
    _expression_stack.emplace(expression.clone());
}

void Encoder::visit(const ir::TimeConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::EnumeratedValue &expression) {
    _expression_stack.emplace(expression.clone());
}

void Encoder::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::VariableAccess &expression) {
    _expression_stack.emplace(expression.clone());
}

void Encoder::visit(const ir::FieldAccess &expression) {
    _expression_stack.emplace(expression.clone());
}

void Encoder::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}
