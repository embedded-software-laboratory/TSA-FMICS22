#include "se/shadow/execution/divergence_evaluator.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/change_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/enumerated_value.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/constant/time_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"

using namespace se::shadow;

const std::string DivergenceEvaluator::CONCRETE_SHADOW_VARIABLE_NAME_PREFIX = "concrete_shadow";

DivergenceEvaluator::DivergenceEvaluator(Solver &solver)
    : _solver(&solver), _shadow_version(0), _context(nullptr), _shadow_processing_mode(ShadowProcessingMode::NONE),
      _expression_stack(std::stack<z3::expr>()) {}

z3::expr DivergenceEvaluator::evaluate(const ir::Expression &expression, const Context &context,
                                       ShadowProcessingMode shadow_processing_mode) {
    _context = &context;
    _shadow_processing_mode = shadow_processing_mode;
    assert(_expression_stack.empty());
    expression.accept(*this);
    assert(_expression_stack.size() == 1);
    z3::expr evaluated_expression = _expression_stack.top();
    _expression_stack.pop();
    return evaluated_expression;
}

void DivergenceEvaluator::reset() {
    _shadow_version = 0;
    _context = nullptr;
    _shadow_processing_mode = ShadowProcessingMode::NONE;
    _expression_stack = std::stack<z3::expr>();
}

void DivergenceEvaluator::visit(const ir::BinaryExpression &expression) {
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
            assert((left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith()) ||
                   (left_evaluated_expression.is_bool() && right_evaluated_expression.is_bool()));
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
        case ir::BinaryExpression::BinaryOperator::SUBTRACT: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression - right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::LESS_THAN: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression < right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::GREATER_THAN: {
            assert(left_evaluated_expression.is_arith() && right_evaluated_expression.is_arith());
            z3::expr z3_expression = (left_evaluated_expression > right_evaluated_expression).simplify();
            _expression_stack.push(z3_expression);
            break;
        }
        case ir::BinaryExpression::BinaryOperator::EXPONENTIATION:
        case ir::BinaryExpression::BinaryOperator::MULTIPLY:
        case ir::BinaryExpression::BinaryOperator::DIVIDE:
        case ir::BinaryExpression::BinaryOperator::MODULO:
        case ir::BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO:
        case ir::BinaryExpression::BinaryOperator::INEQUALITY:
        case ir::BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR:
        default:
            throw std::runtime_error("Unexpected binary operator encountered.");
    }
}

void DivergenceEvaluator::visit(const ir::BooleanToIntegerCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceEvaluator::visit(const ir::ChangeExpression &expression) {
    switch (_shadow_processing_mode) {
        case ShadowProcessingMode::NONE: {
            throw std::runtime_error("Encountered change expression while shadow processing mode is set to \"NONE\".");
        }
        case ShadowProcessingMode::OLD: {
            expression.getOldOperand().accept(*this);
            break;
        }
        case ShadowProcessingMode::NEW: {
            expression.getNewOperand().accept(*this);
            break;
        }
        case ShadowProcessingMode::BOTH: {
            expression.getOldOperand().accept(*this);
            z3::expr old_encoded_expression = _expression_stack.top();
            _expression_stack.pop();
            expression.getNewOperand().accept(*this);
            z3::expr new_encoded_expression = _expression_stack.top();
            _expression_stack.pop();

            unsigned int cycle = _context->getCycle();
            State &state = _context->getState();
            auto *divergent_state = dynamic_cast<DivergentState *>(&state);
            assert(divergent_state != nullptr);
            unsigned int shadow_version = _shadow_version++;
            std::string contextualized_shadow_name = CONCRETE_SHADOW_VARIABLE_NAME_PREFIX + "_" +
                                                     std::to_string(shadow_version) + "__" + std::to_string(cycle);
            divergent_state->setConcreteShadowValuation(contextualized_shadow_name, old_encoded_expression,
                                                        new_encoded_expression);

            if (old_encoded_expression.is_bool()) {
                assert(new_encoded_expression.is_bool());
                z3::expr shadow_expression = _solver->makeBooleanConstant(contextualized_shadow_name);
                _expression_stack.push(shadow_expression);
            } else if (old_encoded_expression.is_int()) {
                assert(new_encoded_expression.is_int());
                z3::expr shadow_expression = _solver->makeIntegerConstant(contextualized_shadow_name);
                _expression_stack.push(shadow_expression);
            } else {
                throw std::runtime_error("Unexpected z3::sort encountered.");
            }
            break;
        }
    }
}

void DivergenceEvaluator::visit(const ir::UnaryExpression &expression) {
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

void DivergenceEvaluator::visit(const ir::BooleanConstant &expression) {
    bool value = expression.getValue();
    _expression_stack.push(_solver->makeBooleanValue(value));
}

void DivergenceEvaluator::visit(const ir::IntegerConstant &expression) {
    int value = expression.getValue();
    _expression_stack.push(_solver->makeIntegerValue(value));
}

void DivergenceEvaluator::visit(const ir::TimeConstant &expression) {
    int value = expression.getValue();
    _expression_stack.push(_solver->makeIntegerValue(value));
}

void DivergenceEvaluator::visit(const ir::EnumeratedValue &expression) {
    int index = expression.getIndex();
    _expression_stack.push(_solver->makeIntegerValue(index));
}

void DivergenceEvaluator::visit(const ir::NondeterministicConstant &expression) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceEvaluator::visit(const ir::Undefined &expression) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceEvaluator::visit(const ir::VariableAccess &expression) {
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    z3::expr evaluated_expression = state.getConcreteValuation(contextualized_name);
    if (_shadow_processing_mode == ShadowProcessingMode::OLD) {
        auto *divergent_state = dynamic_cast<const DivergentState *>(&state);
        assert(divergent_state != nullptr);
        if (containsShadowExpression(*divergent_state, evaluated_expression)) {
            z3::expr lowered_expression =
                    lowerToShadowExpression(*divergent_state, evaluated_expression, ShadowProcessingMode::OLD);
            _expression_stack.push(lowered_expression);
        } else {
            _expression_stack.push(evaluated_expression);
        }
    } else if (_shadow_processing_mode == ShadowProcessingMode::NEW) {
        auto *divergent_state = dynamic_cast<const DivergentState *>(&state);
        assert(divergent_state != nullptr);
        if (containsShadowExpression(*divergent_state, evaluated_expression)) {
            z3::expr lowered_expression =
                    lowerToShadowExpression(*divergent_state, evaluated_expression, ShadowProcessingMode::NEW);
            _expression_stack.push(lowered_expression);
        } else {
            _expression_stack.push(evaluated_expression);
        }
    } else {
        _expression_stack.push(evaluated_expression);
    }
}

void DivergenceEvaluator::visit(const ir::FieldAccess &expression) {
    const Frame &frame = _context->getFrame();
    const State &state = _context->getState();
    std::string name = expression.getName();
    std::string flattened_name = frame.getScope() + "." + name;
    unsigned int cycle = _context->getCycle();
    unsigned int version = state.getHighestVersion(flattened_name);
    std::string contextualized_name = flattened_name + "_" + std::to_string(version) + "__" + std::to_string(cycle);
    z3::expr evaluated_expression = state.getConcreteValuation(contextualized_name);
    if (_shadow_processing_mode == ShadowProcessingMode::OLD) {
        auto *divergent_state = dynamic_cast<const DivergentState *>(&state);
        assert(divergent_state != nullptr);
        if (containsShadowExpression(*divergent_state, evaluated_expression)) {
            z3::expr lowered_expression =
                    lowerToShadowExpression(*divergent_state, evaluated_expression, ShadowProcessingMode::OLD);
            _expression_stack.push(lowered_expression);
        } else {
            _expression_stack.push(evaluated_expression);
        }
    } else if (_shadow_processing_mode == ShadowProcessingMode::NEW) {
        auto *divergent_state = dynamic_cast<const DivergentState *>(&state);
        assert(divergent_state != nullptr);
        if (containsShadowExpression(*divergent_state, evaluated_expression)) {
            z3::expr lowered_expression =
                    lowerToShadowExpression(*divergent_state, evaluated_expression, ShadowProcessingMode::NEW);
            _expression_stack.push(lowered_expression);
        } else {
            _expression_stack.push(evaluated_expression);
        }
    } else {
        _expression_stack.push(evaluated_expression);
    }
}

void DivergenceEvaluator::visit(const ir::IntegerToBooleanCast &expression) {
    throw std::logic_error("Not implemented yet.");
}

void DivergenceEvaluator::visit(const ir::Phi &expression) {
    throw std::logic_error("Not implemented yet.");
}

bool DivergenceEvaluator::containsShadowExpression(const DivergentState &divergent_state,
                                                   const z3::expr &expression) const {
    // XXX check if expression is already a shadow expression, else check if it contains shadow expressions
    const std::map<std::string, std::pair<z3::expr, z3::expr>> &concrete_shadow_valuations =
            divergent_state.getConcreteShadowValuations();
    if (concrete_shadow_valuations.find(expression.to_string()) != concrete_shadow_valuations.end()) {
        return true;
    } else {
        std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
        if (uninterpreted_constants.empty()) {
            assert(expression.is_true() || expression.is_false() || expression.is_numeral());
            return false;
        } else if (uninterpreted_constants.size() == 1) {
            std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
            z3::expr nested_expression = divergent_state.getConcreteValuation(contextualized_name);
            std::vector<z3::expr> nested_uninterpreted_constants =
                    _solver->getUninterpretedConstants(nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
                return false;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    return concrete_shadow_valuations.find(nested_contextualized_name) !=
                           concrete_shadow_valuations.end();
                } else {
                    return containsShadowExpression(divergent_state, nested_expression);
                }
            } else {
                return containsShadowExpression(divergent_state, nested_expression);
            }
        } else {
            bool contains_shadow_expression = false;
            for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
                if (containsShadowExpression(divergent_state, uninterpreted_constant)) {
                    contains_shadow_expression = true;
                    break;
                } else {
                    std::string contextualized_name = uninterpreted_constant.decl().name().str();
                    z3::expr nested_expression = divergent_state.getConcreteValuation(contextualized_name);
                    if (containsShadowExpression(divergent_state, nested_expression)) {
                        contains_shadow_expression = true;
                        break;
                    }
                }
            }
            return contains_shadow_expression;
        }
    }
}

z3::expr DivergenceEvaluator::lowerToShadowExpression(const DivergentState &divergent_state, const z3::expr &expression,
                                                      ShadowProcessingMode shadow_processing_mode) const {
    // XXX check if expression is already a shadow expression, else check if it contains shadow expressions
    const std::map<std::string, std::pair<z3::expr, z3::expr>> &concrete_shadow_valuations =
            divergent_state.getConcreteShadowValuations();
    if (concrete_shadow_valuations.find(expression.to_string()) != concrete_shadow_valuations.end()) {
        if (shadow_processing_mode == ShadowProcessingMode::OLD) {
            return concrete_shadow_valuations.at(expression.to_string()).first;
        } else {
            assert(shadow_processing_mode == ShadowProcessingMode::NEW);
            return concrete_shadow_valuations.at(expression.to_string()).second;
        }
    } else {
        std::vector<z3::expr> uninterpreted_constants = _solver->getUninterpretedConstants(expression);
        if (uninterpreted_constants.empty()) {
            assert(expression.is_true() || expression.is_false() || expression.is_numeral());
            return expression;
        } else if (uninterpreted_constants.size() == 1) {
            std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
            z3::expr nested_expression = divergent_state.getConcreteValuation(contextualized_name);
            std::vector<z3::expr> nested_uninterpreted_constants =
                    _solver->getUninterpretedConstants(nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                assert(nested_expression.is_true() || nested_expression.is_false() || nested_expression.is_numeral());
                return expression;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    return expression;
                } else {
                    return lowerToShadowExpression(divergent_state, nested_expression, shadow_processing_mode);
                }
            } else {
                return lowerToShadowExpression(divergent_state, nested_expression, shadow_processing_mode);
            }
        } else {
            z3::expr lowered_expression = expression;
            for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
                z3::expr lowered_uninterpreted_constant =
                        lowerToShadowExpression(divergent_state, uninterpreted_constant, shadow_processing_mode);
                z3::expr_vector source(_solver->getContext());
                source.push_back(uninterpreted_constant);
                z3::expr_vector destination(_solver->getContext());
                destination.push_back(lowered_uninterpreted_constant);
                lowered_expression = lowered_expression.substitute(source, destination);
            }
            return lowered_expression.simplify();
        }
    }
}