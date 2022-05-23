#include "se/baseline/execution/evaluator.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"

using namespace se::baseline;

Evaluator::Evaluator(Solver &solver) : _solver(&solver), _context(nullptr) {}

z3::expr Evaluator::evaluate(const ir::Expression &expression, const Context &context) {
    _context = &context;
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    z3::expr evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    return evaluated_expression;
}

void Evaluator::visit(const ir::BinaryExpression &expression) {
    expression.getLeftOperand().accept(*this);
    z3::expr left_evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    expression.getRightOperand().accept(*this);
    z3::expr right_evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    switch (expression.getBinaryOperator()) {
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression >= right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::ADD: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression + right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::EQUALITY: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression == right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_AND: {
            assert(left_evaluated_expression.is_bool() && right_evaluated_expression.is_bool());
            z3::expr z3_expression = (left_evaluated_expression && right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_OR: {
            assert(left_evaluated_expression.is_bool() && right_evaluated_expression.is_bool());
            z3::expr z3_expression = (left_evaluated_expression || right_evaluated_expression).simplify();
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
        case ir::BinaryExpression::BinaryOperator::INEQUALITY:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        default:
            throw std::runtime_error("Unexpected binary operator encountered.");
    }
}

void Evaluator::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::ChangeExpression &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::UnaryExpression &expression) {
    expression.getOperand().accept(*this);
    z3::expr evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    switch (expression.getUnaryOperator()) {
        case ir::UnaryExpression::UnaryOperator::NEGATION: {
            assert(evaluated_expression.is_arith());
            z3::expr negated_evaluated_expression = (-evaluated_expression).simplify();
            _expression_stack.push(negated_evaluated_expression);
            break;
        }
        case ir::UnaryExpression::UnaryOperator::UNARY_PLUS: {
            throw std::logic_error("Not implemented yet.");
        }
        case ir::UnaryExpression::UnaryOperator::COMPLEMENT: {
            assert(evaluated_expression.is_bool());
            z3::expr complemented_evaluated_expression = (!evaluated_expression).simplify();
            _expression_stack.push(complemented_evaluated_expression);
            break;
        }
        default:
            throw std::runtime_error("Unexpected unary operator encountered.");
    }
}

void Evaluator::visit(const ir::BooleanConstant &expression) {
    bool value = expression.getValue();
    _expression_stack.push(_solver->makeBooleanValue(value));
}

void Evaluator::visit(const ir::IntegerConstant &expression) {
    int value = expression.getValue();
    _expression_stack.push(_solver->makeIntegerValue(value));
}

void Evaluator::visit(const ir::TimeConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::EnumeratedValue &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::VariableAccess &expression) {
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name, cycle);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    z3::expr evaluated_expression = state.getConcreteValuation(contextualized_name);
    _expression_stack.push(evaluated_expression);
}

void Evaluator::visit(const ir::FieldAccess &expression) {
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name, cycle);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    z3::expr evaluated_expression = state.getConcreteValuation(contextualized_name);
    _expression_stack.push(evaluated_expression);
}

void Evaluator::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Evaluator::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}