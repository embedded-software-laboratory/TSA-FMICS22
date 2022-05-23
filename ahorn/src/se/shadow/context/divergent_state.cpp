#include "se/shadow/context/divergent_state.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::shadow;

DivergentState::DivergentState(const Vertex &vertex, std::map<std::string, z3::expr> concrete_valuations,
                               std::map<std::string, std::pair<z3::expr, z3::expr>> concrete_shadow_valuations,
                               std::map<std::string, z3::expr> symbolic_valuations,
                               std::map<std::string, std::pair<z3::expr, z3::expr>> symbolic_shadow_valuations,
                               std::vector<z3::expr> path_constraint, std::vector<z3::expr> old_path_constraint,
                               std::vector<z3::expr> new_path_constraint,
                               std::map<std::string, unsigned int> flattened_name_to_version)
    : State(vertex, std::move(concrete_valuations), std::move(symbolic_valuations), std::move(path_constraint),
            std::move(flattened_name_to_version)),
      _concrete_shadow_valuations(std::move(concrete_shadow_valuations)),
      _symbolic_shadow_valuations(std::move(symbolic_shadow_valuations)),
      _old_path_constraint(std::move(old_path_constraint)), _new_path_constraint(std::move(new_path_constraint)) {}

std::ostream &DivergentState::print(std::ostream &os) const {
    auto extract_cycle = [](const std::string &contextualized_name) {
        std::size_t cycle_position = contextualized_name.find("__");
        unsigned int cycle = std::stoi(contextualized_name.substr(cycle_position + 2, contextualized_name.size()));
        return cycle;
    };
    std::stringstream str;
    str << "\tstate: ";
    State::print(str);
    str << "\n";
    str << "\tdivergent state: ";
    str << "(\n";
    str << "\t\tconcrete shadow valuations: {";
    if (!_concrete_shadow_valuations.empty()) {
        str << "\n";
        std::map<unsigned int, std::vector<std::string>> pretty_printing_concrete_shadow_expressions;
        for (const auto &p : _concrete_shadow_valuations) {
            std::stringstream pretty_string;
            pretty_string << fmt::format(fmt::fg(fmt::terminal_color::green), p.first) << " -> "
                          << "(" << p.second.first << ", " << p.second.second << ")";
            unsigned int cycle = extract_cycle(p.first);
            auto it = pretty_printing_concrete_shadow_expressions.find(cycle);
            if (it == pretty_printing_concrete_shadow_expressions.end()) {
                pretty_printing_concrete_shadow_expressions.emplace(cycle,
                                                                    std::vector<std::string>({pretty_string.str()}));
            } else {
                it->second.push_back(pretty_string.str());
            }
        }
        for (auto it = pretty_printing_concrete_shadow_expressions.begin();
             it != pretty_printing_concrete_shadow_expressions.end(); ++it) {
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
            if (std::next(it) != pretty_printing_concrete_shadow_expressions.end()) {
                str << ",\n ";
            }
        }
        str << "\n\t\t";
    }
    str << "},\n";
    str << "\t\tsymbolic shadow valuations: {";
    if (!_symbolic_shadow_valuations.empty()) {
        str << "\n";
        std::map<unsigned int, std::vector<std::string>> pretty_printing_symbolic_shadow_expressions;
        for (const auto &p : _symbolic_shadow_valuations) {
            std::stringstream pretty_string;
            pretty_string << fmt::format(fmt::fg(fmt::terminal_color::blue), p.first) << " -> "
                          << "(" << p.second.first << ", " << p.second.second << ")";
            unsigned int cycle = extract_cycle(p.first);
            auto it = pretty_printing_symbolic_shadow_expressions.find(cycle);
            if (it == pretty_printing_symbolic_shadow_expressions.end()) {
                pretty_printing_symbolic_shadow_expressions.emplace(cycle,
                                                                    std::vector<std::string>({pretty_string.str()}));
            } else {
                it->second.push_back(pretty_string.str());
            }
        }
        for (auto it = pretty_printing_symbolic_shadow_expressions.begin();
             it != pretty_printing_symbolic_shadow_expressions.end(); ++it) {
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
            if (std::next(it) != pretty_printing_symbolic_shadow_expressions.end()) {
                str << ",\n ";
            }
        }
        str << "\n\t\t";
    }
    str << "},\n";
    str << "\t\told path constraint: ";
    str << "[";
    for (auto constraint = _old_path_constraint.begin(); constraint != _old_path_constraint.end(); ++constraint) {
        str << *constraint;
        if (std::next(constraint) != _old_path_constraint.end()) {
            str << ", ";
        }
    }
    str << "],\n";
    str << "\t\tnew path constraint: ";
    str << "[";
    for (auto constraint = _new_path_constraint.begin(); constraint != _new_path_constraint.end(); ++constraint) {
        str << *constraint;
        if (std::next(constraint) != _new_path_constraint.end()) {
            str << ", ";
        }
    }
    str << "]\n";
    str << "\t)";
    return os << str.str();
}

const std::map<std::string, std::pair<z3::expr, z3::expr>> &DivergentState::getConcreteShadowValuations() const {
    return _concrete_shadow_valuations;
}

void DivergentState::setConcreteShadowValuation(const std::string &contextualized_name, const z3::expr &old_valuation,
                                                const z3::expr &new_valuation) {
    assert(_concrete_shadow_valuations.find(contextualized_name) == _concrete_shadow_valuations.end());
    _concrete_shadow_valuations.emplace(contextualized_name, std::make_pair(old_valuation, new_valuation));
}

const std::map<std::string, std::pair<z3::expr, z3::expr>> &DivergentState::getSymbolicShadowValuations() const {
    return _symbolic_shadow_valuations;
}

void DivergentState::setSymbolicShadowValuation(const std::string &contextualized_name, const z3::expr &old_valuation,
                                                const z3::expr &new_valuation) {
    assert(_symbolic_shadow_valuations.find(contextualized_name) == _symbolic_shadow_valuations.end());
    _symbolic_shadow_valuations.emplace(contextualized_name, std::make_pair(old_valuation, new_valuation));
}

const std::vector<z3::expr> &DivergentState::getOldPathConstraint() const {
    return _old_path_constraint;
}

void DivergentState::pushOldPathConstraint(const z3::expr &expression) {
    _old_path_constraint.push_back(expression);
}

const std::vector<z3::expr> &DivergentState::getNewPathConstraint() const {
    return _new_path_constraint;
}

void DivergentState::pushNewPathConstraint(const z3::expr &expression) {
    _new_path_constraint.push_back(expression);
}

std::unique_ptr<DivergentState> DivergentState::divergentFork(Solver &solver, const Vertex &vertex, z3::model &model,
                                                              const z3::expr &old_encoded_expression,
                                                              const z3::expr &new_encoded_expression) const {
    auto logger = spdlog::get("Shadow");
    // XXX copy concrete valuations and update valuations from the model, it is not necessarily complete, hence reuse
    // XXX old valuations that were irrelevant for the satisfiability of the fork, i.e., "don't care"
    std::map<std::string, z3::expr> concrete_valuations = _concrete_valuations;
    for (unsigned int i = 0; i < model.size(); ++i) {
        z3::func_decl constant_declaration = model.get_const_decl(i);
        assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
        std::string contextualized_name = constant_declaration.name().str();
        z3::expr interpretation = model.get_const_interp(constant_declaration);
        // XXX update/"overwrite" old valuation
        concrete_valuations.at(contextualized_name) = interpretation;
    }
    SPDLOG_LOGGER_TRACE(logger, "Updating forked divergent state with concrete valuations {} from model.",
                        concrete_valuations);
    std::vector<z3::expr> path_constraint{_path_constraint};
    std::vector<z3::expr> old_path_constraint{_old_path_constraint};
    std::vector<z3::expr> new_path_constraint{_new_path_constraint};
    old_path_constraint.push_back(old_encoded_expression);
    new_path_constraint.push_back(new_encoded_expression);
    return std::make_unique<DivergentState>(vertex, std::move(concrete_valuations), _concrete_shadow_valuations,
                                            _symbolic_valuations, _symbolic_shadow_valuations,
                                            std::move(path_constraint), std::move(old_path_constraint),
                                            std::move(new_path_constraint), _flattened_name_to_version);
}