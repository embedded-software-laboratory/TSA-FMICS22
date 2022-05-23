#include "se/experimental/context/state.h"
#include "se/experimental/expression/concrete_expression.h"
#include "se/experimental/expression/symbolic_expression.h"
#include "se/experimental/z3/manager.h"
#include "utilities/ostream_operators.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <cassert>
#include <set>
#include <sstream>

using namespace se;

State::State(
        const Configuration &configuration, const Vertex &vertex,
        std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                contextualized_name_to_concrete_expression,
        std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                contextualized_name_to_symbolic_expression,
        std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression,
        std::vector<std::shared_ptr<Expression>> path_constraint,
        std::shared_ptr<AssumptionLiteralExpression> assumption_literal,
        std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                 VerificationConditionComparator>
                assumption_literals,
        std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                assumptions,
        std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints,
        std::map<std::string, std::string, VerificationConditionComparator> unknown_over_approximating_summary_literals)
    : _configuration(configuration), _vertex(&vertex),
      _contextualized_name_to_concrete_expression(std::move(contextualized_name_to_concrete_expression)),
      _contextualized_name_to_symbolic_expression(std::move(contextualized_name_to_symbolic_expression)),
      _shadow_name_to_shadow_expression(std::move(shadow_name_to_shadow_expression)),
      _path_constraint(std::move(path_constraint)), _assumption_literal(std::move(assumption_literal)),
      _assumption_literals(std::move(assumption_literals)), _assumptions(std::move(assumptions)),
      _hard_constraints(std::move(hard_constraints)),
      _unknown_over_approximating_summary_literals(std::move(unknown_over_approximating_summary_literals)) {}

std::ostream &State::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\t\tvertex: " << *_vertex << ",\n";
    str << "\t\tconcrete store: {\n";
    std::map<unsigned int, std::vector<std::string>> pretty_printing_concrete_expressions;
    for (const auto &p : _contextualized_name_to_concrete_expression) {
        std::stringstream pretty_string;
        pretty_string << fmt::format(fmt::fg(fmt::terminal_color::green), p.first) << " -> " << *(p.second);
        unsigned int cycle = extractCycle(p.first);
        auto it = pretty_printing_concrete_expressions.find(cycle);
        if (it == pretty_printing_concrete_expressions.end()) {
            pretty_printing_concrete_expressions.emplace(cycle, std::vector<std::string>({pretty_string.str()}));
        } else {
            it->second.push_back(pretty_string.str());
        }
    }
    for (auto it = pretty_printing_concrete_expressions.begin(); it != pretty_printing_concrete_expressions.end();
         ++it) {
        str << "\t\t\t"
            << fmt::format(fmt::fg(fmt::terminal_color::black) | fmt::emphasis::bold,
                           "cycle " + std::to_string(it->first))
            << ": [";
        for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
            str << *it1;
            if (std::next(it1) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]";
        if (std::next(it) != pretty_printing_concrete_expressions.end()) {
            str << ",\n ";
        }
    }
    str << "\n\t\t},\n";
    str << "\t\tsymbolic store: {\n";
    std::map<unsigned int, std::vector<std::string>> pretty_printing_symbolic_expressions;
    for (const auto &p : _contextualized_name_to_symbolic_expression) {
        std::stringstream pretty_string;
        pretty_string << fmt::format(fmt::fg(fmt::terminal_color::blue), p.first) << " -> " << *(p.second);
        unsigned int cycle = extractCycle(p.first);
        auto it = pretty_printing_symbolic_expressions.find(cycle);
        if (it == pretty_printing_symbolic_expressions.end()) {
            pretty_printing_symbolic_expressions.emplace(cycle, std::vector<std::string>({pretty_string.str()}));
        } else {
            it->second.push_back(pretty_string.str());
        }
    }
    for (auto it = pretty_printing_symbolic_expressions.begin(); it != pretty_printing_symbolic_expressions.end();
         ++it) {
        str << "\t\t\t"
            << fmt::format(fmt::fg(fmt::terminal_color::black) | fmt::emphasis::bold,
                           "cycle " + std::to_string(it->first))
            << ": [";
        for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
            str << *it1;
            if (std::next(it1) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]";
        if (std::next(it) != pretty_printing_symbolic_expressions.end()) {
            str << ",\n ";
        }
    }
    str << "\n\t\t},\n";
    switch (_configuration.getEngineMode()) {
        case Configuration::EngineMode::COMPOSITIONAL:
            break;
        case Configuration::EngineMode::SHADOW: {
            str << "\t\tshadow store: [";
            for (auto it = _shadow_name_to_shadow_expression.begin(); it != _shadow_name_to_shadow_expression.end();
                 ++it) {
                str << it->first << " -> " << *it->second;
                if (std::next(it) != _shadow_name_to_shadow_expression.end()) {
                    str << ", ";
                }
            }
            str << "],\n";
            break;
        }
        default:
            throw std::runtime_error("Unexpected engine mode encountered.");
    }
    str << "\t\tpath constraint: " << _path_constraint;
    switch (_configuration.getEncodingMode()) {
        case Configuration::EncodingMode::NONE: {
            str << "\n";
            break;
        }
        case Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            str << ",\n";
            str << "\t\tassumption literal: " << *_assumption_literal << ",\n";
            str << "\t\tassumption literals: {\n";
            for (auto it = _assumption_literals.begin(); it != _assumption_literals.end(); ++it) {
                str << "\t\t\t"
                    << "\"" << it->first << "\""
                    << ": [";
                for (auto assumption_literal_it = it->second.begin(); assumption_literal_it != it->second.end();
                     ++assumption_literal_it) {
                    str << *(*assumption_literal_it);
                    if (std::next(assumption_literal_it) != it->second.end()) {
                        str << ", ";
                    }
                }
                str << "]";
                if (std::next(it) != _assumption_literals.end()) {
                    str << ",\n";
                }
            }
            str << "\n\t\t},\n";
            str << "\t\tassumptions: {";
            if (_assumptions.empty()) {
                str << "},\n";
            } else {
                str << "\n";
                for (auto it = _assumptions.begin(); it != _assumptions.end(); ++it) {
                    str << "\t\t\t"
                        << "\"" << it->first << "\""
                        << ": [";
                    for (auto assumption_literal_it = it->second.begin(); assumption_literal_it != it->second.end();
                         ++assumption_literal_it) {
                        str << *(*assumption_literal_it);
                        if (std::next(assumption_literal_it) != it->second.end()) {
                            str << ", ";
                        }
                    }
                    str << "]";
                    if (std::next(it) != _assumptions.end()) {
                        str << ",\n";
                    }
                }
                str << "\n\t\t},\n";
            }
            str << "\t\thard constraints: {";
            if (_hard_constraints.empty()) {
                str << "}\n";
            } else {
                str << "\n";
                for (auto it = _hard_constraints.begin(); it != _hard_constraints.end(); ++it) {
                    str << "\t\t\t"
                        << "\"" << it->first << "\""
                        << ": [";
                    for (auto valuation = it->second.begin(); valuation != it->second.end(); ++valuation) {
                        str << valuation->first << " = " << valuation->second;
                        if (std::next(valuation) != it->second.end()) {
                            str << ", ";
                        }
                    }
                    str << "]";
                    if (std::next(it) != _hard_constraints.end()) {
                        str << ",\n";
                    }
                }
                str << "\n\t\t},\n";
            }
            str << "\t\tunknown over-approximating summary literals: {";
            if (_unknown_over_approximating_summary_literals.empty()) {
                str << "}\n";
            } else {
                str << "\n";
                for (auto it = _unknown_over_approximating_summary_literals.begin();
                     it != _unknown_over_approximating_summary_literals.end(); ++it) {
                    str << "\t\t\t"
                        << "\"" << it->first << "\""
                        << ": " << it->second;
                    if (std::next(it) != _unknown_over_approximating_summary_literals.end()) {
                        str << ",\n";
                    }
                }
                str << "\n\t\t}\n";
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected encoding mode encountered.");
    }
    str << "\t)";
    return os << str.str();
}

const Vertex &State::getVertex() const {
    return *_vertex;
}

void State::setVertex(const Vertex &vertex) {
    _vertex = &vertex;
}

std::map<std::string, std::shared_ptr<Expression>>::const_iterator State::concreteStoreBegin() const {
    return std::begin(_contextualized_name_to_concrete_expression);
}

std::map<std::string, std::shared_ptr<Expression>>::const_iterator State::concreteStoreEnd() const {
    return std::end(_contextualized_name_to_concrete_expression);
}

const std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator> &
State::getConcreteStore() const {
    return _contextualized_name_to_concrete_expression;
}

std::shared_ptr<Expression> State::getConcreteExpression(const std::string &contextualized_name) const {
    assert(_contextualized_name_to_concrete_expression.find(contextualized_name) !=
           _contextualized_name_to_concrete_expression.end());
    return _contextualized_name_to_concrete_expression.at(contextualized_name);
}

void State::setConcreteExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression) {
    assert(_contextualized_name_to_concrete_expression.find(contextualized_name) ==
           _contextualized_name_to_concrete_expression.end());
    _contextualized_name_to_concrete_expression.emplace(contextualized_name, std::move(expression));
}

void State::updateConcreteExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression) {
    auto it = _contextualized_name_to_concrete_expression.find(contextualized_name);
    if (it == _contextualized_name_to_concrete_expression.end()) {
        throw std::runtime_error("Can not update concrete expression because entry does not exist.");
    } else {
        it->second = std::move(expression);
    }
}

std::map<std::string, std::shared_ptr<Expression>>::const_iterator State::symbolicStoreBegin() const {
    return std::begin(_contextualized_name_to_symbolic_expression);
}

std::map<std::string, std::shared_ptr<Expression>>::const_iterator State::symbolicStoreEnd() const {
    return std::end(_contextualized_name_to_symbolic_expression);
}

const std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator> &
State::getSymbolicStore() const {
    return _contextualized_name_to_symbolic_expression;
}

std::shared_ptr<Expression> State::getSymbolicExpression(const std::string &contextualized_name) const {
    assert(_contextualized_name_to_symbolic_expression.find(contextualized_name) !=
           _contextualized_name_to_symbolic_expression.end());
    return _contextualized_name_to_symbolic_expression.at(contextualized_name);
}

void State::setSymbolicExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression) {
    assert(_contextualized_name_to_symbolic_expression.find(contextualized_name) ==
           _contextualized_name_to_symbolic_expression.end());
    _contextualized_name_to_symbolic_expression.emplace(contextualized_name, std::move(expression));
}

const std::map<std::string, std::shared_ptr<ShadowExpression>> &State::getShadowStore() const {
    return _shadow_name_to_shadow_expression;
}

std::shared_ptr<ShadowExpression> State::getShadowExpression(const std::string &shadow_name) const {
    assert(_shadow_name_to_shadow_expression.find(shadow_name) != _shadow_name_to_shadow_expression.end());
    return _shadow_name_to_shadow_expression.at(shadow_name);
}

void State::setShadowExpression(const std::string &shadow_name, std::shared_ptr<ShadowExpression> shadow_expression) {
    assert(_shadow_name_to_shadow_expression.find(shadow_name) == _shadow_name_to_shadow_expression.end());
    _shadow_name_to_shadow_expression.emplace(shadow_name, std::move(shadow_expression));
}

const std::vector<std::shared_ptr<Expression>> &State::getPathConstraint() const {
    return _path_constraint;
}

void State::pushPathConstraint(std::shared_ptr<Expression> expression) {
    _path_constraint.push_back(std::move(expression));
}

void State::clearPathConstraint() {
    _path_constraint.clear();
}

std::shared_ptr<AssumptionLiteralExpression> State::getAssumptionLiteral() const {
    return _assumption_literal;
}

void State::setAssumptionLiteral(std::shared_ptr<AssumptionLiteralExpression> assumption_literal) {
    _assumption_literal = std::move(assumption_literal);
}

const std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>, VerificationConditionComparator>
        &State::getAssumptionLiterals() const {
    return _assumption_literals;
}

const std::vector<std::shared_ptr<AssumptionLiteralExpression>> &
State::getAssumptionLiterals(const std::string &assumption_literal_name) const {
    auto it = _assumption_literals.find(assumption_literal_name);
    if (it == _assumption_literals.end()) {
        throw std::runtime_error("Assumption literal does not exist.");
    } else {
        return it->second;
    }
}

void State::pushAssumptionLiteral(std::string assumption_literal_name,
                                  std::shared_ptr<AssumptionLiteralExpression> assumption_literal) {
    auto it = _assumption_literals.find(assumption_literal_name);
    if (it == _assumption_literals.end()) {
        _assumption_literals.emplace(
                std::move(assumption_literal_name),
                std::vector<std::shared_ptr<AssumptionLiteralExpression>>{std::move(assumption_literal)});
    } else {
        // XXX check if assumption literal already exists, if yes, do not add it
        if (std::find_if(_assumption_literals.at(assumption_literal_name).begin(),
                         _assumption_literals.at(assumption_literal_name).end(),
                         [assumption_literal](const auto &existing_assumption_literal) {
                             return assumption_literal->getZ3Expression().to_string() ==
                                    existing_assumption_literal->getZ3Expression().to_string();
                         }) == _assumption_literals.at(assumption_literal_name).end()) {
            it->second.push_back(std::move(assumption_literal));
        }
    }
}

const std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator> &
State::getAssumptions() const {
    return _assumptions;
}

const std::vector<std::shared_ptr<SymbolicExpression>> &
State::getAssumptions(const std::string &assumption_literal_name) const {
    return _assumptions.at(assumption_literal_name);
}

void State::pushAssumption(std::string assumption_literal_name, std::shared_ptr<Expression> assumption) {
    std::shared_ptr<SymbolicExpression> symbolic_assumption = std::dynamic_pointer_cast<SymbolicExpression>(assumption);
    assert(symbolic_assumption != nullptr);
    auto it = _assumptions.find(assumption_literal_name);
    if (it == _assumptions.end()) {
        _assumptions.emplace(std::move(assumption_literal_name),
                             std::vector<std::shared_ptr<SymbolicExpression>>{std::move(symbolic_assumption)});
    } else {
        it->second.push_back(std::move(symbolic_assumption));
    }
}

const std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> &
State::getHardConstraints() const {
    return _hard_constraints;
}

const std::map<std::string, z3::expr> &State::getHardConstraints(const std::string &assumption_literal_name) const {
    return _hard_constraints.at(assumption_literal_name);
}

void State::pushHardConstraint(std::string assumption_literal_name, const std::string &contextualized_name,
                               const z3::expr &z3_expression) {
    auto it = _hard_constraints.find(assumption_literal_name);
    if (it == _hard_constraints.end()) {
        _hard_constraints.emplace(std::move(assumption_literal_name),
                                  std::map<std::string, z3::expr>{std::make_pair(contextualized_name, z3_expression)});
    } else {
        it->second.emplace(contextualized_name, z3_expression);
    }
}

const std::map<std::string, std::string, VerificationConditionComparator> &
State::getUnknownOverApproximatingSummaryLiterals() const {
    return _unknown_over_approximating_summary_literals;
}

void State::pushUnknownOverApproximatingSummaryLiteral(const std::string &caller_assumption_literal_name,
                                                       const std::string &callee_assumption_literal_name) {
    if (_unknown_over_approximating_summary_literals.count(caller_assumption_literal_name) == 0) {
        _unknown_over_approximating_summary_literals.emplace(caller_assumption_literal_name,
                                                             callee_assumption_literal_name);
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

std::unique_ptr<State> State::fork(Manager &manager, z3::model &z3_model) {
    std::unique_ptr<State> state =
            std::make_unique<State>(_configuration, *_vertex, _contextualized_name_to_concrete_expression,
                                    _contextualized_name_to_symbolic_expression, _shadow_name_to_shadow_expression,
                                    _path_constraint, _assumption_literal, _assumption_literals, _assumptions,
                                    _hard_constraints, _unknown_over_approximating_summary_literals);


    std::chrono::time_point<std::chrono::system_clock> begin_time_point = std::chrono::system_clock::now();

    // XXX update
    std::map<std::string, std::shared_ptr<ConcreteExpression>> contextualized_name_to_concrete_expression;
    for (unsigned int i = 0; i < z3_model.size(); ++i) {
        z3::func_decl constant_declaration = z3_model.get_const_decl(i);
        assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
        std::string contextualized_name = constant_declaration.name().str();
        z3::expr interpretation = z3_model.get_const_interp(constant_declaration);
        contextualized_name_to_concrete_expression.emplace(contextualized_name,
                                                           std::make_shared<ConcreteExpression>(interpretation));
    }

    bool new_evaluation = true;
    if (new_evaluation) {
        // std::cout << "before completion: " << z3_model << std::endl;

        for (const auto &unaffected_concrete_valuations : state->getConcreteStore()) {
            if (contextualized_name_to_concrete_expression.count(unaffected_concrete_valuations.first) == 0) {
                std::shared_ptr<Expression> concrete_expression = unaffected_concrete_valuations.second;
                z3::expr z3_concrete_expression = concrete_expression->getZ3Expression();
                if (z3_concrete_expression.is_bool()) {
                    z3::expr z3_variable = manager.makeBooleanConstant(unaffected_concrete_valuations.first);
                    z3::func_decl z3_variable_declaration = z3_variable.decl();
                    z3_model.add_const_interp(z3_variable_declaration, z3_concrete_expression);
                } else if (z3_concrete_expression.is_int()) {
                    z3::expr z3_variable = manager.makeIntegerConstant(unaffected_concrete_valuations.first);
                    z3::func_decl z3_variable_declaration = z3_variable.decl();
                    z3_model.add_const_interp(z3_variable_declaration, z3_concrete_expression);
                } else {
                    throw std::runtime_error("Unsupported z3::sort encountered.");
                }
            }
        }

        // TODO (27.01.2022): Evaluate using the model (i.e., let z3 do the work) instead of concretely reevaluating
        //  recursively by myself!
        for (const auto &contextualized_name_to_symbolic_expression : state->getSymbolicStore()) {
            z3::expr evaluated_expression =
                    z3_model.eval(contextualized_name_to_symbolic_expression.second->getZ3Expression());
            state->updateConcreteExpression(contextualized_name_to_symbolic_expression.first,
                                            std::make_shared<ConcreteExpression>(evaluated_expression));
        }

        // std::cout << "after completion: " << z3_model << std::endl;
    } else {

        for (const auto &contextualized_name_to_symbolic_expression : state->getSymbolicStore()) {
            z3::expr evaluated_expression =
                    z3_model.eval(contextualized_name_to_symbolic_expression.second->getZ3Expression());
            // std::cout << "reevaluating " << contextualized_name_to_symbolic_expression.first << " resulting in "
            //          << evaluated_expression << std::endl;
        }


        // gather the contextualized names that require a potential reevaluation (because they depend on the concrete
        // valuation of an updated expression)
        std::set<std::string> contextualized_names;
        for (const auto &contextualized_name_to_symbolic_expression : state->getSymbolicStore()) {
            std::string contextualized_name = contextualized_name_to_symbolic_expression.first;
            if (contextualized_name_to_concrete_expression.find(contextualized_name) ==
                contextualized_name_to_concrete_expression.end()) {
                // no concrete valuation was derived from the model, hence, this symbolic expression may be dependant on
                // the new concrete valuation
                contextualized_names.emplace(contextualized_name);
            }
        }

        // reevaluate symbolic expressions that contain values from the model
        std::map<std::string, std::shared_ptr<Expression>> contextualized_name_to_reevaluated_concrete_expression;
        for (const std::string &contextualized_name : contextualized_names) {
            std::shared_ptr<Expression> reevaluated_expression =
                    state->reevaluateExpressionConcretely(manager, *state->getSymbolicExpression(contextualized_name),
                                                          contextualized_name_to_concrete_expression);
            contextualized_name_to_reevaluated_concrete_expression.emplace(contextualized_name, reevaluated_expression);
        }

        for (auto &v : contextualized_name_to_concrete_expression) {
            assert(contextualized_name_to_reevaluated_concrete_expression.find(v.first) ==
                   contextualized_name_to_reevaluated_concrete_expression.end());
            assert(v.second->getKind() == Expression::Kind::CONCRETE_EXPRESSION);
            contextualized_name_to_reevaluated_concrete_expression.emplace(v.first, v.second);
        }

        for (auto &v : contextualized_name_to_reevaluated_concrete_expression) {
            state->updateConcreteExpression(v.first, std::move(v.second));
        }
    }

    using namespace std::literals;
    auto elapsed_time = (std::chrono::system_clock::now() - begin_time_point) / 1ms;
    std::cout << "Elapsed time (state forking): " << elapsed_time << "ms" << std::endl;

    return state;
}

std::shared_ptr<Expression>
State::reevaluateExpressionConcretely(const Manager &manager, Expression &expression,
                                      const std::map<std::string, std::shared_ptr<ConcreteExpression>>
                                              &contextualized_name_to_concrete_expression) const {
    auto logger = spdlog::get("Symbolic Execution");
    SPDLOG_LOGGER_TRACE(logger, "Reevaluating: {}", expression);

    const z3::expr &z3_expression = expression.getZ3Expression();
    std::vector<z3::expr> uninterpreted_constants = manager.getUninterpretedConstantsFromZ3Expression(z3_expression);
    if (uninterpreted_constants.empty()) {
        z3::expr z3_simplified_expression = z3_expression.simplify();
        assert((z3_simplified_expression.is_true() || z3_simplified_expression.is_false()) ||
               z3_simplified_expression.is_numeral());
        return std::make_shared<ConcreteExpression>(z3_simplified_expression);
    } else if (uninterpreted_constants.size() == 1) {
        std::string contextualized_name = uninterpreted_constants.at(0).decl().name().str();
        auto it = contextualized_name_to_concrete_expression.find(contextualized_name);
        if (it == contextualized_name_to_concrete_expression.end()) {
            std::shared_ptr<Expression> nested_expression = getSymbolicExpression(contextualized_name);
            const z3::expr &z3_nested_expression = nested_expression->getZ3Expression();
            std::vector<z3::expr> nested_uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_nested_expression);
            if (nested_uninterpreted_constants.empty()) {
                z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                        z3_expression, uninterpreted_constants.at(0), z3_nested_expression);
                z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                assert((z3_simplified_substituted_expression.is_true() ||
                        z3_simplified_substituted_expression.is_false()) ||
                       z3_simplified_substituted_expression.is_numeral());
                return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
            } else if (nested_uninterpreted_constants.size() == 1) {
                std::string nested_contextualized_name = nested_uninterpreted_constants.at(0).decl().name().str();
                auto nested_it = contextualized_name_to_concrete_expression.find(nested_contextualized_name);
                if (nested_it == contextualized_name_to_concrete_expression.end()) {
                    if (contextualized_name == nested_contextualized_name) {
                        // substitute with concrete value from the concrete store of the state
                        std::shared_ptr<Expression> concrete_expression = getConcreteExpression(contextualized_name);
                        const z3::expr &z3_concrete_expression = concrete_expression->getZ3Expression();
                        z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                                z3_expression, uninterpreted_constants.at(0), z3_concrete_expression);
                        z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                        return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
                    } else {
                        // recursively descend
                        std::shared_ptr<Expression> reevaluated_expression = reevaluateExpressionConcretely(
                                manager, *nested_expression, contextualized_name_to_concrete_expression);
                        z3::expr z3_reevaluated_expression = reevaluated_expression->getZ3Expression();
                        z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                                z3_expression, uninterpreted_constants.at(0), z3_reevaluated_expression);
                        z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                        return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
                    }
                } else {
                    std::shared_ptr<ConcreteExpression> nested_concrete_expression = nested_it->second;
                    const z3::expr &z3_nested_concrete_expression = nested_concrete_expression->getZ3Expression();
                    z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                            z3_expression, uninterpreted_constants.at(0), z3_nested_concrete_expression);
                    z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                    return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
                }
            } else {
                // e.g., P.y_3__0 -> (ite (>= P.x_0__0 32) P.y_2__0 P.y_1__0)
                std::shared_ptr<Expression> reevaluated_expression = reevaluateExpressionConcretely(
                        manager, *nested_expression, contextualized_name_to_concrete_expression);
                z3::expr z3_reevaluated_expression = reevaluated_expression->getZ3Expression();
                z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                        z3_expression, uninterpreted_constants.at(0), z3_reevaluated_expression);
                z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
                return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
            }
        } else {
            // substitute with concrete value from the model
            std::shared_ptr<ConcreteExpression> concrete_expression = it->second;
            const z3::expr &z3_concrete_expression = concrete_expression->getZ3Expression();
            z3::expr z3_substituted_expression = manager.substituteZ3Expression(
                    z3_expression, uninterpreted_constants.at(0), z3_concrete_expression);
            z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
            return std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
        }
    } else {
        // (ite (>= P.x_0__1 32)
        //     (+ 1 (ite (>= P.x_0__0 32) 1 0))
        //     (ite (>= P.x_0__0 32) 1 0))
        z3::expr z3_substituted_expression = z3_expression.simplify();
        for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
            std::string contextualized_name = uninterpreted_constant.decl().name().str();
            std::shared_ptr<Expression> nested_expression = getSymbolicExpression(contextualized_name);
            std::shared_ptr<Expression> reevaluated_expression = reevaluateExpressionConcretely(
                    manager, *nested_expression, contextualized_name_to_concrete_expression);
            z3::expr z3_lowered_expression = reevaluated_expression->getZ3Expression();
            z3_substituted_expression = manager.substituteZ3Expression(z3_substituted_expression,
                                                                       uninterpreted_constant, z3_lowered_expression);
        }
        z3::expr z3_simplified_substituted_expression = z3_substituted_expression.simplify();
        std::shared_ptr<Expression> concrete_expression =
                std::make_shared<ConcreteExpression>(z3_simplified_substituted_expression);
        return concrete_expression;
    }
}

unsigned int State::extractVersion(const std::string &contextualized_name) const {
    std::size_t version_position = contextualized_name.find('_');
    std::size_t cycle_position = contextualized_name.find("__");
    unsigned int version =
            std::stoi(contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
    return version;
}

unsigned int State::extractCycle(const std::string &contextualized_name) const {
    std::size_t cycle_position = contextualized_name.find("__");
    unsigned int cycle = std::stoi(contextualized_name.substr(cycle_position + 2, contextualized_name.size()));
    return cycle;
}

void State::removeIntermediateVersions(const Manager &manager, unsigned int cycle) {
    auto logger = spdlog::get("Symbolic Execution");

    // keeps track of self-references
    std::set<std::string> self_references;
    // XXX can't erase "normally" as stores have custom comparator
    // TODO (28.01.2022): Could be improved by assuming the following invariant: concrete and symbolic
    //  stores always refer to the same contextualized names, further improvements: getting minimum/maximum versions
    //  from stores?
    std::set<std::string> removable_intermediate_versions;
    for (auto it = _contextualized_name_to_symbolic_expression.begin();
         it != _contextualized_name_to_symbolic_expression.end(); ++it) {
        std::string contextualized_name = it->first;
        if (cycle == extractCycle(contextualized_name)) {
            z3::expr z3_expression = it->second->getZ3Expression();
            std::vector<z3::expr> uninterpreted_constants =
                    manager.getUninterpretedConstantsFromZ3Expression(z3_expression);
            if (uninterpreted_constants.size() == 1) {
                if (contextualized_name == uninterpreted_constants.at(0).decl().name().str()) {
                    std::cout << "self-reference: " << contextualized_name << std::endl;
                    self_references.insert(contextualized_name);
                }
            }
            if ((self_references.count(contextualized_name) == 0) &&
                (extractVersion(contextualized_name) !=
                 getMinimumVersionFromSymbolicStore(contextualized_name, cycle))) {
                removable_intermediate_versions.emplace(contextualized_name);
            }
        }
    }
    for (const std::string &contextualized_name : removable_intermediate_versions) {
        _contextualized_name_to_symbolic_expression.erase(
                _contextualized_name_to_symbolic_expression.find(contextualized_name));
        _contextualized_name_to_concrete_expression.erase(
                _contextualized_name_to_concrete_expression.find(contextualized_name));
    }

    SPDLOG_LOGGER_TRACE(logger, "Removed the following intermediate variable versions: {}.",
                        removable_intermediate_versions);
}

bool State::isShadowConstant(const std::string &name) const {
    return _shadow_name_to_shadow_expression.find(name) != _shadow_name_to_shadow_expression.end();
}

unsigned int State::getMaximumVersionFromConcreteStore(const std::string &flattened_name, unsigned int cycle) const {
    return getMaximumVersion(true, flattened_name, cycle);
}

unsigned int State::getMinimumVersionFromConcreteStore(const std::string &flattened_name, unsigned int cycle) const {
    return getMinimumVersion(true, flattened_name, cycle);
}

unsigned int State::getMaximumVersionFromSymbolicStore(const std::string &flattened_name, unsigned int cycle) const {
    return getMaximumVersion(false, flattened_name, cycle);
}

unsigned int State::getMinimumVersionFromSymbolicStore(const std::string &flattened_name, unsigned int cycle) const {
    return getMinimumVersion(false, flattened_name, cycle);
}

unsigned int State::getMinimumVersion(bool from_concrete_store, const std::string &flattened_name,
                                      unsigned int cycle) const {
    unsigned int minimum_version = getMaximumVersion(from_concrete_store, flattened_name, cycle);
    if (from_concrete_store) {
        for (const auto &contextualized_name_to_concrete_expression : _contextualized_name_to_concrete_expression) {
            std::string contextualized_name = contextualized_name_to_concrete_expression.first;
            std::size_t version_position = contextualized_name.find('_');
            std::size_t cycle_position = contextualized_name.find("__");
            if (flattened_name == contextualized_name.substr(0, version_position)) {
                if (cycle == std::stoul(contextualized_name.substr(cycle_position + 2, contextualized_name.size()))) {
                    unsigned int version = std::stoul(
                            contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
                    if (minimum_version > version) {
                        minimum_version = version;
                    }
                }
            }
        }
    } else {
        for (const auto &contextualized_name_to_symbolic_expression : _contextualized_name_to_symbolic_expression) {
            std::string contextualized_name = contextualized_name_to_symbolic_expression.first;
            std::size_t version_position = contextualized_name.find('_');
            std::size_t cycle_position = contextualized_name.find("__");
            if (flattened_name == contextualized_name.substr(0, version_position)) {
                if (cycle == std::stoul(contextualized_name.substr(cycle_position + 2, contextualized_name.size()))) {
                    unsigned int version = std::stoul(
                            contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
                    if (minimum_version > version) {
                        minimum_version = version;
                    }
                }
            }
        }
    }
    return minimum_version;
}

unsigned int State::getMaximumVersion(bool from_concrete_store, const std::string &flattened_name,
                                      unsigned int cycle) const {
    unsigned int maximum_version = 0;
    if (from_concrete_store) {
        for (const auto &contextualized_name_to_concrete_expression : _contextualized_name_to_concrete_expression) {
            std::string contextualized_name = contextualized_name_to_concrete_expression.first;
            std::size_t version_position = contextualized_name.find('_');
            std::size_t cycle_position = contextualized_name.find("__");
            if (flattened_name == contextualized_name.substr(0, version_position)) {
                if (cycle == std::stoul(contextualized_name.substr(cycle_position + 2, contextualized_name.size()))) {
                    unsigned int version = std::stoul(
                            contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
                    if (maximum_version < version) {
                        maximum_version = version;
                    }
                }
            }
        }
    } else {
        for (const auto &contextualized_name_to_symbolic_expression : _contextualized_name_to_symbolic_expression) {
            std::string contextualized_name = contextualized_name_to_symbolic_expression.first;
            std::size_t version_position = contextualized_name.find('_');
            std::size_t cycle_position = contextualized_name.find("__");
            if (flattened_name == contextualized_name.substr(0, version_position)) {
                if (cycle == std::stoul(contextualized_name.substr(cycle_position + 2, contextualized_name.size()))) {
                    unsigned int version = std::stoul(
                            contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
                    if (maximum_version < version) {
                        maximum_version = version;
                    }
                }
            }
        }
    }
    return maximum_version;
}