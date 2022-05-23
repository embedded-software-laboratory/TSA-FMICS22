#include "se/experimental/context/context.h"
#include "se/experimental/expression/symbolic_expression.h"
#include "se/experimental/z3/manager.h"
#include "utilities/ostream_operators.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se;

Context::Context(unsigned int cycle, std::unique_ptr<State> state, std::deque<std::shared_ptr<Frame>> frame_stack)
    : _cycle(cycle), _state(std::move(state)), _frame_stack(std::move(frame_stack)) {}

std::ostream &Context::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tcycle: " << _cycle << ",\n";
    str << "\tstate: " << *_state << ",\n";
    str << "\tframe stack: " << _frame_stack;
    str << ")";
    return os << str.str();
}

unsigned int Context::getCycle() const {
    return _cycle;
}

void Context::setCycle(unsigned int cycle) {
    _cycle = cycle;
}

State &Context::getState() {
    return *_state;
}

const State &Context::getState() const {
    return *_state;
}

const std::deque<std::shared_ptr<Frame>> &Context::getFrameStack() const {
    return _frame_stack;
}

unsigned int Context::getFrameDepth() const {
    return _frame_stack.size();
}

Frame &Context::getFrame() const {
    assert(!_frame_stack.empty());
    return *_frame_stack.back();
}

Frame &Context::getMainFrame() const {
    assert(!_frame_stack.empty());
    return *_frame_stack.front();
}

void Context::pushFrame(std::shared_ptr<Frame> frame) {
    _frame_stack.push_back(std::move(frame));
}

void Context::popFrame() {
    assert(!_frame_stack.empty());
    _frame_stack.pop_back();
}

void Context::pushLocalPathConstraint(std::shared_ptr<Expression> expression) {
    assert(!_frame_stack.empty());
    _frame_stack.back()->pushLocalPathConstraint(std::move(expression));
}

std::string Context::getFlattenedName(const std::string &name) const {
    std::stringstream str;
    assert(!_frame_stack.empty());
    const Frame &frame = *_frame_stack.back();
    str << frame.getScope() << "." << name;
    return str.str();
}

std::string Context::getDecontextualizedName(const std::string &contextualized_name) const {
    std::size_t position = contextualized_name.find('_');
    assert(position != std::string::npos);
    return contextualized_name.substr(0, position);
}

std::unique_ptr<Context> Context::fork(Manager &manager, z3::model &z3_model) const {
    std::deque<std::shared_ptr<Frame>> frame_stack;
    for (const auto &frame : _frame_stack) {
        frame_stack.emplace_back(std::make_shared<Frame>(frame->getCfg(), frame->getScope(), frame->getLabel()));
    }
    std::unique_ptr<State> state = _state->fork(manager, z3_model);
    return std::make_unique<Context>(_cycle, std::move(state), std::move(frame_stack));
}

std::set<std::string> Context::extractNecessaryHardConstraints(const Manager &manager, const z3::expr &z3_expression) {
    std::set<std::string> necessary_hard_constraints;
    std::vector<z3::expr> uninterpreted_constants = manager.getUninterpretedConstantsFromZ3Expression(z3_expression);
    if (uninterpreted_constants.empty()) {
        return necessary_hard_constraints;
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        unsigned int cycle = _state->extractCycle(contextualized_name);
        if (_cycle < cycle) {
            throw std::runtime_error("The global cycle count should never be lower than the local cycle count.");
        } else if (_cycle == cycle) {
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
            std::vector<z3::expr> nested_uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                necessary_hard_constraints.emplace(contextualized_name);
                return necessary_hard_constraints;
            } else if (nested_uninterpreted_constants.size() == 1) {
                if (contextualized_name == nested_uninterpreted_constants.at(0).decl().name().str()) {
                    // "whole-program" input must be skipped, as they are unconstrained
                    return necessary_hard_constraints;
                } else {
                    // recursively descend
                    necessary_hard_constraints.emplace(contextualized_name);
                    for (const std::string &nested_contextualized_name :
                         extractNecessaryHardConstraints(manager, z3_nested_expression)) {
                        necessary_hard_constraints.emplace(nested_contextualized_name);
                    }
                    return necessary_hard_constraints;
                }
            } else {
                // recursively descend
                necessary_hard_constraints.emplace(contextualized_name);
                for (const std::string &nested_nested_contextualized_name :
                     extractNecessaryHardConstraints(manager, z3_nested_expression)) {
                    necessary_hard_constraints.emplace(nested_nested_contextualized_name);
                }
                return necessary_hard_constraints;
            }
        } else {
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
            std::vector<z3::expr> nested_uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                throw std::logic_error("Not implemented yet.");
            } else if (nested_uninterpreted_constants.size() == 1) {
                if (contextualized_name == nested_uninterpreted_constants.at(0).decl().name().str()) {
                    // "whole-program" inputs from prior cycles should also be kept unconstrained
                    return necessary_hard_constraints;
                } else {
                    throw std::logic_error("Not implemented yet.");
                }
            } else {
                throw std::logic_error("Not implemented yet.");
            }
        }
    } else {
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            for (const std::string &contextualized_name :
                 extractNecessaryHardConstraints(manager, uninterpreted_constant)) {
                necessary_hard_constraints.emplace(contextualized_name);
            }
        }
    }
    return necessary_hard_constraints;
}

// e.g.,    (not (>= (ite (>= P.x_0__1 32) (+ P.y_0__1 1) P.y_0__1) 3)),
//          P.x_0__1 -> P.x_0__1,
//          P.y_0__1 -> (ite (>= P.x_0__0 32) 1 0)
std::shared_ptr<Expression>
Context::lowerExpression(const Manager &manager, const std::shared_ptr<Expression> &expression,
                         std::map<unsigned int, std::vector<z3::expr>> &id_to_uninterpreted_constants,
                         std::map<unsigned int, std::shared_ptr<Expression>> &id_to_lowered_expression) const {
    const z3::expr &z3_simplified_expression = expression->getZ3Expression().simplify();
    if (id_to_lowered_expression.count(z3_simplified_expression.id()) != 0) {
        return id_to_lowered_expression.at(z3_simplified_expression.id());
    }
    std::vector<z3::expr> uninterpreted_constants =
            manager.getUninterpretedConstantsFromZ3Expression(z3_simplified_expression);
    /*
    std::vector<z3::expr> uninterpreted_constants;
    if (id_to_uninterpreted_constants.count(z3_simplified_expression.id()) == 0) {
        uninterpreted_constants = manager.getUninterpretedConstantsFromZ3Expression(z3_simplified_expression);
        id_to_uninterpreted_constants.emplace(z3_simplified_expression.id(), uninterpreted_constants);
    } else {
        uninterpreted_constants = id_to_uninterpreted_constants.at(z3_simplified_expression.id());
    }
     */
    if (uninterpreted_constants.empty()) {
        assert((z3_simplified_expression.is_true() || z3_simplified_expression.is_false()) ||
               z3_simplified_expression.is_numeral());
        std::shared_ptr<ConcreteExpression> lowered_expression =
                std::make_shared<ConcreteExpression>(z3_simplified_expression);
        id_to_lowered_expression.emplace(z3_simplified_expression.id(), lowered_expression);
        return lowered_expression;
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        unsigned int cycle = _state->extractCycle(contextualized_name);
        if (_cycle < cycle) {
            throw std::runtime_error("The global cycle count should never be lower than the local cycle count.");
        } else if (_cycle == cycle) {
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
            std::vector<z3::expr> nested_uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
            /*
            std::vector<z3::expr> nested_uninterpreted_constants;
            if (id_to_uninterpreted_constants.count(z3_nested_expression.id()) == 0) {
                nested_uninterpreted_constants =
                        manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
                id_to_uninterpreted_constants.emplace(z3_nested_expression.id(), nested_uninterpreted_constants);
            } else {
                nested_uninterpreted_constants = id_to_uninterpreted_constants.at(z3_nested_expression.id());
            }
             */
            if (nested_uninterpreted_constants.empty()) {
                z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                        z3_simplified_expression, uninterpreted_constants.at(0), z3_nested_expression);
                z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                assert((z3_simplified_substituted_expression.is_true() ||
                        z3_simplified_substituted_expression.is_false()) ||
                       z3_simplified_substituted_expression.is_numeral());
                std::shared_ptr<ConcreteExpression> lowered_expression =
                        std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
                id_to_lowered_expression.emplace(z3_simplified_substituted_expression.id(), lowered_expression);
                return lowered_expression;
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                // XXX a "whole-program" input variable can still be lowered, as it can be written during execution,
                // hence we need to check for self-references as our stop condition
                if (contextualized_name == nested_contextualized_name) {
                    return expression;
                } else {
                    // recursively descend
                    std::shared_ptr<Expression> lowered_expression = lowerExpression(
                            manager, nested_expression, id_to_uninterpreted_constants, id_to_lowered_expression);
                    z3::expr z3_lowered_expression = lowered_expression->getZ3Expression();
                    z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                            z3_simplified_expression, uninterpreted_constants.at(0), z3_lowered_expression);
                    z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                    std::shared_ptr<SymbolicExpression> simplified_substituted_expression =
                            std::make_shared<SymbolicExpression>(z3_simplified_substituted_expression);
                    id_to_lowered_expression.emplace(z3_simplified_substituted_expression.id(),
                                                     simplified_substituted_expression);
                    return simplified_substituted_expression;
                }
            } else {
                // e.g., P.y_3__0 -> (ite (>= P.x_0__0 32) P.y_2__0 P.y_1__0)
                std::shared_ptr<Expression> lowered_expression = lowerExpression(
                        manager, nested_expression, id_to_uninterpreted_constants, id_to_lowered_expression);
                z3::expr z3_lowered_expression = lowered_expression->getZ3Expression();
                z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                        z3_simplified_expression, uninterpreted_constants.at(0), z3_lowered_expression);
                z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                std::shared_ptr<SymbolicExpression> simplified_substituted_expression =
                        std::make_shared<SymbolicExpression>(z3_simplified_substituted_expression);
                id_to_lowered_expression.emplace(z3_simplified_substituted_expression.id(),
                                                 simplified_substituted_expression);
                return simplified_substituted_expression;
            }
        } else {
            // e.g., (not (>= (ite (>= P.x_0__0 32) 1 0) 3)) in cycle 1
            // variables from prior cycles should be kept fixed as they are already lowered to the minimum version.
            // as this expression only consists of one variable and this one variable is from a prior cycle, simply
            // return the original expression; no more lowering (or substitution) needed
            id_to_lowered_expression.emplace(uninterpreted_constants.at(0).id(), expression);
            return expression;
        }
    } else {
        // e.g., (ite (>= P.x_0__0 32) P.y_2__0 P.y_1__0)
        z3::expr z3_substituted_expression = z3_simplified_expression;
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            std::shared_ptr<Expression> lowered_expression = lowerExpression(
                    manager, nested_expression, id_to_uninterpreted_constants, id_to_lowered_expression);
            z3::expr z3_lowered_expression = lowered_expression->getZ3Expression();
            z3_substituted_expression = manager.substituteZ3Expression(z3_substituted_expression,
                                                                       uninterpreted_constant, z3_lowered_expression);
        }
        z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
        std::shared_ptr<Expression> lowered_expression =
                std::make_shared<SymbolicExpression>(z3_simplified_substituted_expression);
        id_to_lowered_expression.emplace(z3_simplified_substituted_expression.id(), lowered_expression);
        return lowered_expression;
    }
}

// XXX propagation in order to retrieve a concrete expression
std::shared_ptr<Expression> Context::propagateConstantExpressions(const Manager &manager,
                                                                  const Expression &expression) {
    const z3::expr &z3_expression = expression.getZ3Expression();
    std::vector<z3::expr> uninterpreted_constants = manager.getUninterpretedConstantsFromZ3Expression(z3_expression);
    if (uninterpreted_constants.empty()) {
        // XXX simplify, in case expression is a term (+ 0 2)
        z3::expr z3_simplified_expression = z3_expression.simplify();
        assert((z3_simplified_expression.is_true() || z3_simplified_expression.is_false()) ||
               z3_simplified_expression.is_numeral());
        return std::make_shared<ConcreteExpression>(z3_simplified_expression);
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        unsigned int cycle = _state->extractCycle(contextualized_name);
        if (_cycle < cycle) {
            throw std::runtime_error("The global cycle count should never be lower than the local cycle count.");
        } else if (_cycle == cycle) {
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
            std::vector<z3::expr> nested_uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                throw std::logic_error("Not implemented yet.");
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                if (contextualized_name == nested_contextualized_name) {
                    std::shared_ptr<Expression> nested_concrete_expression =
                            _state->getConcreteExpression(contextualized_name);
                    z3::expr z3_substituted_expression =
                            manager.substituteZ3Expression(z3_expression, uninterpreted_constants.at(0),
                                                           nested_concrete_expression->getZ3Expression());
                    z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                    return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
                } else {
                    throw std::logic_error("Not implemented yet.");
                }
            } else {
                throw std::logic_error("Not implemented yet.");
            }
            /*
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                unsigned int nested_cycle = _state->extractCycle(nested_contextualized_name);
                if (_cycle < nested_cycle) {
                    throw std::logic_error("Not implemented yet.");
                } else if (_cycle == nested_cycle) {
                    if (contextualized_name == nested_contextualized_name) {
                        // XXX self-reference is either a "whole-program" input or a constant
                        unsigned int version = _state->extractVersion(contextualized_name);
                        return version == 0 ? std::make_shared<SymbolicExpression>(z3_expression)
                                            : _state->getConcreteExpression(contextualized_name);
                    } else {
                        throw std::logic_error("Not implemented yet.");
                    }
                } else {
                    return std::make_shared<SymbolicExpression>(z3_expression);
                }
            } else {
                throw std::logic_error("Not implemented yet.");
            }
            */
        } else {
            // value from prior cycle, retrieve the concrete valuation and substituted accordingly
            std::shared_ptr<Expression> nested_concrete_expression = _state->getConcreteExpression(contextualized_name);
            z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                    z3_expression, uninterpreted_constants.at(0), nested_concrete_expression->getZ3Expression());
            z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
            return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
        }
    } else {
        // e.g., (ite (>= P.x_0__1 32)
        //          (+ 1 (ite (>= P.x_0__0 32) 1 0))
        //          (ite (>= P.x_0__0 32) 1 0))
        z3::expr z3_substituted_expression = z3_expression.simplify();
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            // XXX this assumes that the nested_expression is already lowered!
            std::shared_ptr<Expression> propagated_expression =
                    propagateConstantExpressions(manager, *nested_expression);
            z3::expr z3_propagated_expression = propagated_expression->getZ3Expression();
            z3_substituted_expression = manager.substituteZ3Expression(
                    z3_substituted_expression, uninterpreted_constant, z3_propagated_expression);
        }
        z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
        std::shared_ptr<Expression> concrete_expression =
                std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
        return concrete_expression;
    }
}

bool Context::containsWholeProgramInput(const Manager &manager, const Expression &expression) const {
    const z3::expr &z3_expression = expression.getZ3Expression();
    std::vector<z3::expr> uninterpreted_constants = manager.getUninterpretedConstantsFromZ3Expression(z3_expression);
    if (uninterpreted_constants.empty()) {
        // XXX e.g., if TRUE/FALse/experimental/1/0 then...
        assert(z3_expression.is_true() || z3_expression.is_false() || z3_expression.is_numeral());
        return false;
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
        const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
        std::vector<z3::expr> nested_uninterpreted_constants =
                manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
        if (uninterpreted_constants.empty()) {
            throw std::logic_error("Not implemented yet.");
        } else if (nested_uninterpreted_constants.size() == 1) {
            std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
            // XXX potential self-reference
            if (contextualized_name == nested_contextualized_name) {
                return manager.isWholeProgramInput(getDecontextualizedName(contextualized_name));
            } else {
                return containsWholeProgramInput(manager, *nested_expression);
            }
        } else {
            return containsWholeProgramInput(manager, *nested_expression);
        }
    } else {
        bool contains_whole_program_input = false;
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            std::shared_ptr<Expression> nested_expression = _state->getSymbolicExpression(contextualized_name);
            contains_whole_program_input = containsWholeProgramInput(manager, *nested_expression);
            if (contains_whole_program_input) {
                break;
            }
        }
        return contains_whole_program_input;
    }
}