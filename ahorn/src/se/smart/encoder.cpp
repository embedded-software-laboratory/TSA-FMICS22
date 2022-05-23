#include "se/smart/encoder.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/variable_access.h"
#include "ir/expression/field_access.h"
#include "pass/tac/encoder.h"
#include "se/ahorn/execution/encoder.h"


using namespace se::smart;

Encoder::Encoder(Manager &manager) : _manager(&manager) {}

z3::expr Encoder::encode(const ir::Expression &expression, Context &context) {
    _context = &context;
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    z3::expr encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    return encoded_expression;
}

void Encoder::visit(const ir::BinaryExpression &expression) {
    expression.getLeftOperand().accept(*this);
    z3::expr left_encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    expression.getRightOperand().accept(*this);
    z3::expr right_encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    switch (expression.getBinaryOperator()) {
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO: {
            assert(left_encoded_expression.is_arith() && right_encoded_expression.is_arith());
            z3::expr z3_expression = (left_encoded_expression >= right_encoded_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::ADD: {
            assert(left_encoded_expression.is_arith() && right_encoded_expression.is_arith());
            z3::expr z3_expression = (left_encoded_expression + right_encoded_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::EXPONENTIATION:
        case ir::BinaryExpression::BinaryOperator::MULTIPLY:
        case ir::BinaryExpression::BinaryOperator::DIVIDE:
        case ir::BinaryExpression::BinaryOperator::MODULO:
        case ir::BinaryExpression::BinaryOperator::SUBTRACT:
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN:
        case ir::BinaryExpression::BinaryOperator::LESS_THAN:
        case ir::BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO:
        case ir::BinaryExpression::BinaryOperator::EQUALITY:
        case ir::BinaryExpression::BinaryOperator::INEQUALITY:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_AND:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_OR:
        default:
            throw std::runtime_error("Unexpected binary operator encountered.");
    }
}

void Encoder::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::ChangeExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::UnaryExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::BooleanConstant &expression) {
    bool value = expression.getValue();
    _expression_stack.push(_manager->makeBooleanValue(value));
}

void Encoder::visit(const ir::IntegerConstant &expression) {
    int value = expression.getValue();
    _expression_stack.push(_manager->makeIntegerValue(value));
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
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name, cycle, false);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    std::shared_ptr<ir::Variable> variable = expression.getVariable();
    z3::expr encoded_expression = _manager->makeConstant(contextualized_name, variable->getDataType());
    _expression_stack.push(encoded_expression);
}

void Encoder::visit(const ir::FieldAccess &expression) {
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name, cycle, false);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    const ir::VariableAccess &variable_access = expression.getVariableAccess();
    std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
    z3::expr encoded_expression = _manager->makeConstant(contextualized_name, variable->getDataType());
    _expression_stack.push(encoded_expression);
}

void Encoder::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}