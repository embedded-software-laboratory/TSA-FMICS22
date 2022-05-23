#include "se/experimental/z3/manager.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/type/elementary_type.h"
#include "ir/type/safety_type.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se;

Manager::Manager()
    : _z3_context(std::make_unique<z3::context>()), _flattened_name_to_version(std::map<std::string, unsigned int>()),
      _whole_program_inputs(std::set<std::string>()), _random_number_generator(108) {
    // RNG
    _int_distribution = std::uniform_int_distribution<int>(-32768, 32767);
    _bool_distribution = std::uniform_int_distribution<int>(0, 1);
    _time_distribution = std::uniform_int_distribution<int>(0, 65535);
    _random_number_generator.discard(70000);
    // https://github.com/Z3Prover/z3/blob/master/src/ast/pp_params.pyg
    z3::set_param("pp.max_width", 1000); // 80 default, max width in pretty printer
    z3::set_param("pp.max_ribbon", 1000);// 80 max. ribbon (width - indentation) in pretty printer
    z3::set_param("pp.max_depth", 1000); // max. term depth (when pretty printing SMT2 terms/formulas)
    // min. size for creating an alias for a shared term (when pretty printing SMT2 terms/formulas)
    z3::set_param("pp.min_alias_size", 1000);
    z3::set_param("pp.fixed_indent", true);
    z3::set_param("pp.single_line", true);
    z3::set_param("pp.simplify_implies", true);
}

z3::context &Manager::getZ3Context() const {
    return *_z3_context;
}

unsigned int Manager::getVersion(const std::string &flattened_name) const {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    return _flattened_name_to_version.at(flattened_name);
}

void Manager::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

bool Manager::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

z3::expr Manager::makeBooleanValue(bool value) {
    return _z3_context->bool_val(value);
}

z3::expr Manager::makeIntegerValue(int value) {
    return _z3_context->int_val(value);
}

z3::expr Manager::makeDefaultValue(const ir::DataType &data_type) {
    switch (data_type.getKind()) {
        case ir::DataType::Kind::ELEMENTARY_TYPE: {
            switch (dynamic_cast<const ir::ElementaryType &>(data_type).getType()) {
                case ir::ElementaryType::Type::INT: {
                    return makeIntegerValue(0);
                }
                case ir::ElementaryType::Type::BOOL: {
                    return makeBooleanValue(false);
                }
                case ir::ElementaryType::Type::TIME: {
                    // XXX TIME is modeled as integer with resolution of 1 ms
                    return makeIntegerValue(0);
                }
                case ir::ElementaryType::Type::REAL:
                    // XXX fallthrough
                default:
                    throw std::logic_error("Unsupported elementary type.");
            }
        }
        case ir::DataType::Kind::SAFETY_TYPE: {
            if (const auto *safety_type = dynamic_cast<const ir::SafetyType *>(&data_type)) {
                return makeBooleanValue(false);
            } else {
                throw std::runtime_error("Unsupported safety type.");
            }
        }
        case ir::DataType::Kind::ENUMERATED_TYPE: {
            return makeIntegerValue(0);
        }
        case ir::DataType::Kind::DERIVED_TYPE:
        case ir::DataType::Kind::INCONCLUSIVE_TYPE:
        case ir::DataType::Kind::SIMPLE_TYPE:
        default:
            throw std::logic_error("Unsupported data type.");
    }
}

z3::expr Manager::makeRandomValue(const ir::DataType &data_type) {
    switch (data_type.getKind()) {
        case ir::DataType::Kind::ELEMENTARY_TYPE: {
            switch (dynamic_cast<const ir::ElementaryType &>(data_type).getType()) {
                case ir::ElementaryType::Type::INT: {
                    return makeIntegerValue(_int_distribution(_random_number_generator));
                }
                case ir::ElementaryType::Type::BOOL: {
                    return makeBooleanValue(_bool_distribution(_random_number_generator));
                }
                case ir::ElementaryType::Type::TIME: {
                    return makeIntegerValue(_time_distribution(_random_number_generator));
                }
                case ir::ElementaryType::Type::REAL:
                    // XXX fallthrough
                default:
                    throw std::runtime_error("Unsupported elementary type.");
            }
        }
        case ir::DataType::Kind::SAFETY_TYPE:
        case ir::DataType::Kind::DERIVED_TYPE:
        case ir::DataType::Kind::ENUMERATED_TYPE:
        case ir::DataType::Kind::INCONCLUSIVE_TYPE:
        case ir::DataType::Kind::SIMPLE_TYPE:
        default:
            throw std::runtime_error("Unsupported data type.");
    }
}

z3::expr Manager::makeValue(const ir::Constant &constant) {
    switch (constant.getKind()) {
        case ir::Expression::Kind::BOOLEAN_CONSTANT: {
            return makeBooleanValue(dynamic_cast<const ir::BooleanConstant &>(constant).getValue());
        }
        case ir::Expression::Kind::INTEGER_CONSTANT: {
            return makeIntegerValue(dynamic_cast<const ir::IntegerConstant &>(constant).getValue());
        }
        case ir::Expression::Kind::ENUMERATED_VALUE_CONSTANT:
        case ir::Expression::Kind::TIME_CONSTANT:
        case ir::Expression::Kind::NONDETERMINISTIC_CONSTANT:
        default:
            throw std::logic_error("Not implemented yet.");
    }
}

z3::expr Manager::makeBooleanConstant(const std::string &contextualized_name) {
    return _z3_context->bool_const(contextualized_name.c_str());
}

z3::expr Manager::makeIntegerConstant(const std::string &contextualized_name) {
    return _z3_context->int_const(contextualized_name.c_str());
}

z3::expr Manager::makeConstant(const std::string &contextualized_name, const ir::DataType &data_type) {
    switch (data_type.getKind()) {
        case ir::DataType::Kind::ELEMENTARY_TYPE: {
            switch (dynamic_cast<const ir::ElementaryType &>(data_type).getType()) {
                case ir::ElementaryType::Type::INT: {
                    return makeIntegerConstant(contextualized_name);
                }
                case ir::ElementaryType::Type::BOOL: {
                    return makeBooleanConstant(contextualized_name);
                }
                case ir::ElementaryType::Type::TIME: {
                    // XXX TIME is modeled as integer with resolution of 1 ms
                    return makeIntegerConstant(contextualized_name);
                }
                case ir::ElementaryType::Type::REAL:
                    // XXX fallthrough
                default:
                    throw std::logic_error("Unsupported elementary type.");
            }
        }
        case ir::DataType::Kind::SAFETY_TYPE: {
            if (const auto *safety_type = dynamic_cast<const ir::SafetyType *>(&data_type)) {
                return makeBooleanConstant(contextualized_name);
            } else {
                throw std::runtime_error("Unsupported safety type.");
            }
        }
        case ir::DataType::Kind::ENUMERATED_TYPE: {
            return makeIntegerConstant(contextualized_name);
        }
        case ir::DataType::Kind::INCONCLUSIVE_TYPE:
        case ir::DataType::Kind::SIMPLE_TYPE:
        default:
            throw std::logic_error("Unsupported data type.");
    }
}

std::pair<z3::check_result, boost::optional<z3::model>> Manager::check(const z3::expr_vector &z3_expressions) {
    z3::tactic tactic = z3::tactic(*_z3_context, "simplify") & z3::tactic(*_z3_context, "solve-eqs") &
                        z3::tactic(*_z3_context, "smt");
    z3::solver z3_solver = tactic.mk_solver();
    // XXX create a backtracking point
    z3_solver.push();
    for (const z3::expr &z3_expression : z3_expressions) {
        z3_solver.add(z3_expression);
    }
    z3::check_result z3_result = z3_solver.check();
    std::pair<z3::check_result, boost::optional<z3::model>> result(z3_result, boost::none);
    switch (z3_result) {
        case z3::unsat: {
            // XXX expressions are unsatisfiable
            break;
        }
        case z3::sat: {
            z3::model z3_model = z3_solver.get_model();
            result.second = z3_model;
            break;
        }
        case z3::unknown:
        default:
            throw std::logic_error("Invalid z3::check_result.");
    }
    // XXX backtrack
    z3_solver.pop();
    return result;
}

z3::expr Manager::substituteZ3Expression(z3::expr z3_expression, const z3::expr &z3_source_expression,
                                         const z3::expr &z3_destination_expression) const {
    z3::expr_vector source(*_z3_context);
    source.push_back(z3_source_expression);
    z3::expr_vector destination(*_z3_context);
    destination.push_back(z3_destination_expression);
    z3::expr z3_substituted_expression = z3_expression.substitute(source, destination);
    return z3_substituted_expression;
}

// TODO (24.01.2022): Problem: We can't ask for getConcrete/SymbolicExpression for "inline" expressions in, e.g.,
//  branch condition, e.g., change(true, false) -> this does not have a variable associated. Can we resolve this
//  without relying on Three-Address-Code (TAC, introducing a fresh boolean variable for the condition of the branch)?
bool Manager::containsShadowExpression(const Context &context, const z3::expr &z3_expression, bool concrete) {
    const State &state = context.getState();
    std::vector<z3::expr> uninterpreted_constants = getUninterpretedConstantsFromZ3Expression(z3_expression);
    bool contains_shadow_expression = false;
    for (const z3::expr &uninterpreted_constant : uninterpreted_constants) {
        std::string contextualized_name = uninterpreted_constant.to_string();
        std::size_t shadow_position =
                concrete ? contextualized_name.find("cshadow") : contextualized_name.find("sshadow");
        if (shadow_position == std::string::npos) {
            std::shared_ptr<Expression> nested_expression = concrete ? state.getConcreteExpression(contextualized_name)
                                                                     : state.getSymbolicExpression(contextualized_name);
            z3::expr z3_nested_expression = nested_expression->getZ3Expression();
            contains_shadow_expression = containsShadowExpression(context, z3_nested_expression);
            if (contains_shadow_expression) {
                break;
            }
        } else {
            contains_shadow_expression = true;
            break;
        }
    }
    return contains_shadow_expression;
}

// TODO (27.01.2022): We need to use memoization/caching.
std::vector<z3::expr> Manager::getUninterpretedConstantsFromZ3Expression(const z3::expr &expression) const {
    std::vector<bool> visited;
    std::vector<z3::expr> uninterpreted_constants;
    visit(visited, uninterpreted_constants, expression);
    return uninterpreted_constants;
}

void Manager::visit(std::vector<bool> &visited, std::vector<z3::expr> &uninterpreted_constants,
                    const z3::expr &expression) const {
    if (visited.size() <= expression.id()) {
        visited.resize(expression.id() + 1, false);
    }
    if (visited[expression.id()]) {
        return;
    }
    visited[expression.id()] = true;
    auto it = std::find_if(uninterpreted_constants.begin(), uninterpreted_constants.end(),
                           [expression](const z3::expr &e) { return expression.id() == e.id(); });
    if (it != uninterpreted_constants.end()) {
        // only consider one occurrence of the same expression
        return;
    }
    if (expression.is_app()) {
        unsigned number_of_arguments = expression.num_args();
        for (unsigned i = 0; i < number_of_arguments; ++i) {
            visit(visited, uninterpreted_constants, expression.arg(i));
        }
        z3::func_decl f = expression.decl();
        // XXX The following explanation is taken from: https://stackoverflow.com/questions/12448583/z3-const-declaration
        // XXX Z3 uses the term "variable" for universal and existential variables. Quantifier free formulas do not
        // XXX contain variables, only constants. In the formula "x + 1 > 0", both, "x" and "1", are constants. However,
        // XXX "x" is an uninterpreted constant and "1" is an interpreted constant, i.e, the meaning of "1" is fixed,
        // XXX but Z3 is free to assign an interpretation for "x" in order to make the formula satisfiable.
        if (f.decl_kind() == Z3_decl_kind::Z3_OP_UNINTERPRETED) {
            uninterpreted_constants.push_back(expression);
        }
    } else {
        throw std::logic_error("Invalid expression encountered during visit.");
    }
}

void Manager::initialize(const Cfg &cfg) {
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _flattened_name_to_version.emplace(std::move(flattened_name), 0);
    }
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _whole_program_inputs.emplace(std::move(flattened_name));
    }
}