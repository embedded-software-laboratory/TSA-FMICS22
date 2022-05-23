#include "se/ahorn/z3/solver.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/constant/time_constant.h"
#include "ir/type/elementary_type.h"
#include "ir/type/safety_type.h"

using namespace se::ahorn;

Solver::Solver()
    : _context(std::make_unique<z3::context>()), _random_number_generator(108),
      _int_distribution(std::uniform_int_distribution<int>(-32768, 32767)),
      _bool_distribution(std::uniform_int_distribution<int>(0, 1)),
      _time_distribution(std::uniform_int_distribution<int>(0, 65535)) {
    // RNG
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

z3::context &Solver::getContext() {
    return *_context;
}

z3::expr Solver::makeBooleanValue(bool value) {
    return _context->bool_val(value);
}

z3::expr Solver::makeIntegerValue(int value) {
    return _context->int_val(value);
}

z3::expr Solver::makeDefaultValue(const ir::DataType &data_type) {
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

z3::expr Solver::makeRandomValue(const ir::DataType &data_type) {
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
        case ir::DataType::Kind::SAFETY_TYPE: {
            // XXX safety-type defaults to false
            return makeBooleanValue(false);
        }
        case ir::DataType::Kind::DERIVED_TYPE:
        case ir::DataType::Kind::ENUMERATED_TYPE:
        case ir::DataType::Kind::INCONCLUSIVE_TYPE:
        case ir::DataType::Kind::SIMPLE_TYPE:
        default:
            throw std::runtime_error("Unsupported data type.");
    }
}

z3::expr Solver::makeValue(const ir::Constant &constant) {
    switch (constant.getKind()) {
        case ir::Expression::Kind::BOOLEAN_CONSTANT: {
            return makeBooleanValue(dynamic_cast<const ir::BooleanConstant &>(constant).getValue());
        }
        case ir::Expression::Kind::INTEGER_CONSTANT: {
            return makeIntegerValue(dynamic_cast<const ir::IntegerConstant &>(constant).getValue());
        }
        case ir::Expression::Kind::ENUMERATED_VALUE_CONSTANT:
        case ir::Expression::Kind::TIME_CONSTANT: {
            // XXX TIME_CONSTANT is modeled as integer value
            return makeIntegerValue(dynamic_cast<const ir::TimeConstant &>(constant).getValue());
        }
        case ir::Expression::Kind::NONDETERMINISTIC_CONSTANT:
        default:
            throw std::logic_error("Not implemented yet.");
    }
}

z3::expr Solver::makeBooleanConstant(const std::string &contextualized_name) {
    return _context->bool_const(contextualized_name.c_str());
}

z3::expr Solver::makeIntegerConstant(const std::string &contextualized_name) {
    return _context->int_const(contextualized_name.c_str());
}

z3::expr Solver::makeConstant(const std::string &contextualized_name, const ir::DataType &data_type) {
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

std::pair<z3::check_result, boost::optional<z3::model>> Solver::check(const z3::expr_vector &expressions) {
    z3::tactic tactic =
            z3::tactic(*_context, "simplify") & z3::tactic(*_context, "solve-eqs") & z3::tactic(*_context, "smt");
    z3::solver solver = tactic.mk_solver();
    // XXX add backtracking point
    solver.push();
    solver.add(expressions);
    z3::check_result check_result = solver.check();
    std::pair<z3::check_result, boost::optional<z3::model>> result(check_result, boost::none);
    switch (check_result) {
        case z3::unsat: {
            // XXX expressions are unsatisfiable
            break;
        }
        case z3::sat: {
            z3::model model = solver.get_model();
            result.second = model;
            break;
        }
        case z3::unknown: {
            // XXX fall-through
        }
        default:
            throw std::runtime_error("Unexpected z3::check_result encountered.");
    }
    // XXX drop learned clauses
    solver.pop();
    return result;
}

std::vector<z3::expr> Solver::getUninterpretedConstants(const z3::expr &expression) const {
    std::vector<bool> visited;
    std::vector<z3::expr> uninterpreted_constants;
    visit(visited, uninterpreted_constants, expression);
    return uninterpreted_constants;
}

void Solver::visit(std::vector<bool> &visited, std::vector<z3::expr> &uninterpreted_constants,
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