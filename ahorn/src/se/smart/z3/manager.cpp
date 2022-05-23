#include "se/smart/z3/manager.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/type/elementary_type.h"
#include "ir/type/safety_type.h"

using namespace se::smart;

Manager::Manager()
    : _z3_context(std::make_unique<z3::context>()), _random_number_generator(108) {
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

std::pair<z3::check_result, boost::optional<z3::model>> Manager::check(const z3::expr_vector &expressions) {
    z3::tactic tactic = z3::tactic(*_z3_context, "simplify") & z3::tactic(*_z3_context, "solve-eqs") &
                        z3::tactic(*_z3_context, "smt");
    z3::solver z3_solver = tactic.mk_solver();
    // XXX add backtracking point
    z3_solver.push();
    for (const z3::expr &expression : expressions) {
        z3_solver.add(expression);
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
            throw std::runtime_error("Unexpected z3::check_result encountered.");
    }
    // XXX drop learned clauses
    z3_solver.pop();
    return result;
}