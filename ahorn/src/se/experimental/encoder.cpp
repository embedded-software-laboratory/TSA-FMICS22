#include "se/experimental/encoder.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/change_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "se/experimental/expression/shadow_expression.h"

using namespace se;

Encoder::Encoder(const Configuration &configuration, Manager &manager)
    : _configuration(configuration), _shadow_index(0), _manager(&manager), _context(nullptr),
      _expression_stack(std::stack<std::shared_ptr<Expression>>()) {}

std::shared_ptr<Expression> Encoder::encode(const ir::Expression &expression, Context &context) {
    _context = &context;
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    std::shared_ptr<Expression> encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    return encoded_expression;
}

void Encoder::visit(const ir::BinaryExpression &expression) {
    expression.getLeftOperand().accept(*this);
    std::shared_ptr<Expression> left_encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    expression.getRightOperand().accept(*this);
    std::shared_ptr<Expression> right_encoded_expression = _expression_stack.top();
    _expression_stack.pop();
    switch (expression.getBinaryOperator()) {
        case ir::BinaryExpression::BinaryOperator::ADD: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression + right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::SUBTRACT: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression - right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression > right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression >= right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::EQUALITY: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert((left_z3_expression.is_arith() && right_z3_expression.is_arith()) ||
                   (left_z3_expression.is_bool() && right_z3_expression.is_bool()));
            z3::expr z3_expression = left_z3_expression == right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression <= right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::MULTIPLY: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression * right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::LESS_THAN: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_arith() && right_z3_expression.is_arith());
            z3::expr z3_expression = left_z3_expression < right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_AND: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_bool() && right_z3_expression.is_bool());
            z3::expr z3_expression = (left_z3_expression && right_z3_expression).simplify();
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_OR: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert(left_z3_expression.is_bool() && right_z3_expression.is_bool());
            z3::expr z3_expression = (left_z3_expression || right_z3_expression).simplify();
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::INEQUALITY: {
            const z3::expr &left_z3_expression = left_encoded_expression->getZ3Expression();
            const z3::expr &right_z3_expression = right_encoded_expression->getZ3Expression();
            assert((left_z3_expression.is_arith() && right_z3_expression.is_arith()) ||
                   (left_z3_expression.is_bool() && right_z3_expression.is_bool()));
            z3::expr z3_expression = left_z3_expression != right_z3_expression;
            std::shared_ptr<SymbolicExpression> encoded_expression =
                    std::make_shared<SymbolicExpression>(z3_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::EXPONENTIATION:
        case ir::BinaryExpression::BinaryOperator::DIVIDE:
        case ir::BinaryExpression::BinaryOperator::MODULO:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        default:
            throw std::logic_error("Not implemented yet.");
    }
}

void Encoder::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::ChangeExpression &expression) {
    switch (_configuration.getShadowProcessingMode()) {
        case Configuration::ShadowProcessingMode::NONE: {
            throw std::logic_error("Shadow processing mode set to \"NONE\", but expression is of type "
                                   "ir::ChangeExpression.");
        }
        case Configuration::ShadowProcessingMode::OLD: {
            expression.getOldOperand().accept(*this);
            break;
        }
        case Configuration::ShadowProcessingMode::NEW: {
            expression.getNewOperand().accept(*this);
            break;
        }
        case Configuration::ShadowProcessingMode::BOTH: {
            expression.getOldOperand().accept(*this);
            std::shared_ptr<Expression> old_encoded_expression = _expression_stack.top();
            _expression_stack.pop();
            expression.getNewOperand().accept(*this);
            std::shared_ptr<Expression> new_encoded_expression = _expression_stack.top();
            _expression_stack.pop();

            unsigned int cycle = _context->getCycle();
            State &state = _context->getState();
            std::shared_ptr<ShadowExpression> encoded_expression = nullptr;
            std::string shadow_name = "sshadow_" + std::to_string(_shadow_index++) + "__" + std::to_string(cycle);
            if (old_encoded_expression->getZ3Expression().get_sort().is_bool()) {
                assert(new_encoded_expression->getZ3Expression().get_sort().is_bool());
                encoded_expression = std::make_shared<ShadowExpression>(_manager->makeBooleanConstant(shadow_name),
                                                                        std::move(old_encoded_expression),
                                                                        std::move(new_encoded_expression));
            } else if (old_encoded_expression->getZ3Expression().get_sort().is_int()) {
                assert(new_encoded_expression->getZ3Expression().get_sort().is_int());
                encoded_expression = std::make_shared<ShadowExpression>(_manager->makeIntegerConstant(shadow_name),
                                                                        std::move(old_encoded_expression),
                                                                        std::move(new_encoded_expression));
            } else {
                throw std::runtime_error("Unexpected expression sort.");
            }
            assert(encoded_expression != nullptr);

            state.setShadowExpression(shadow_name, encoded_expression);
            _expression_stack.push(encoded_expression);
            break;
        }
        default:
            throw std::runtime_error("Invalid shadow processing mode configured.");
    }
}

void Encoder::visit(const ir::UnaryExpression &expression) {
    expression.getOperand().accept(*this);
    std::shared_ptr<Expression> evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    switch (expression.getUnaryOperator()) {
        case ir::UnaryExpression::UnaryOperator::NEGATION: {
            const z3::expr &z3_evaluated_expression = evaluated_expression->getZ3Expression();
            assert(z3_evaluated_expression.is_arith());
            z3::expr z3_negated_expression = (-z3_evaluated_expression).simplify();
            std::shared_ptr<SymbolicExpression> negated_expression =
                    std::make_shared<SymbolicExpression>(z3_negated_expression);
            _expression_stack.push(negated_expression);
            break;
        }
        case ir::UnaryExpression::UnaryOperator::UNARY_PLUS:
            throw std::logic_error("Not implemented yet.");
        case ir::UnaryExpression::UnaryOperator::COMPLEMENT: {
            const z3::expr &z3_evaluated_expression = evaluated_expression->getZ3Expression();
            assert(z3_evaluated_expression.is_bool());
            z3::expr z3_complemented_expression = (!z3_evaluated_expression).simplify();
            std::shared_ptr<SymbolicExpression> complemented_expression =
                    std::make_shared<SymbolicExpression>(z3_complemented_expression);
            _expression_stack.push(complemented_expression);
            break;
        }
        default:
            throw std::runtime_error("Unexpected unary operator encountered.");
    }
}

void Encoder::visit(const ir::BooleanConstant &expression) {
    bool value = expression.getValue();
    std::shared_ptr<ConcreteExpression> encoded_expression =
            std::make_shared<ConcreteExpression>(_manager->makeBooleanValue(value));
    _expression_stack.push(encoded_expression);
}

void Encoder::visit(const ir::IntegerConstant &expression) {
    int value = expression.getValue();
    std::shared_ptr<ConcreteExpression> encoded_expression =
            std::make_shared<ConcreteExpression>(_manager->makeIntegerValue(value));
    _expression_stack.push(encoded_expression);
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
    std::string name = expression.getName();
    std::string flattened_name = _context->getFlattenedName(name);
    unsigned int cycle = _context->getCycle();
    const State &state = _context->getState();
    unsigned int version = state.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    std::shared_ptr<ir::Variable> variable = expression.getVariable();
    std::shared_ptr<Expression> encoded_expression =
            std::make_shared<SymbolicExpression>(_manager->makeConstant(contextualized_name, variable->getDataType()));
    _expression_stack.push(encoded_expression);
}

void Encoder::visit(const ir::FieldAccess &expression) {
    std::string name = expression.getName();
    std::string flattened_name = _context->getFlattenedName(name);
    unsigned int cycle = _context->getCycle();
    const State &state = _context->getState();
    unsigned int version = state.getMaximumVersionFromSymbolicStore(flattened_name, cycle);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    const ir::VariableAccess &variable_access = expression.getVariableAccess();
    std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
    std::shared_ptr<Expression> encoded_expression =
            std::make_shared<SymbolicExpression>(_manager->makeConstant(contextualized_name, variable->getDataType()));
    _expression_stack.push(encoded_expression);
}

void Encoder::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void Encoder::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}
