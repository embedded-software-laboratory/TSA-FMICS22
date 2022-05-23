#include "se/smart/context/state.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"

using namespace se::smart;

State::State(const Vertex &vertex, store_t concrete_valuations, store_t symbolic_valuations)
    : _vertex(&vertex), _concrete_valuations(std::move(concrete_valuations)),
      _symbolic_valuations(std::move(symbolic_valuations)), _path_constraint() {}

std::ostream &State::print(std::ostream &os) const {
    auto extract_cycle = [](const std::string &contextualized_name) {
        std::size_t cycle_position = contextualized_name.find("__");
        unsigned int cycle = std::stoi(contextualized_name.substr(cycle_position + 2, contextualized_name.size()));
        return cycle;
    };
    std::stringstream str;
    str << "(\n";
    str << "\t\tvertex: " << *_vertex << ",\n";
    str << "\t\tconcrete valuations: {\n";
    std::map<unsigned int, std::map<unsigned int, std::vector<std::string>>> pretty_printing_concrete_expressions;
    for (const auto &concrete_valuations_at_k : _concrete_valuations) {
        unsigned int k = concrete_valuations_at_k.first;
        for (const auto &concrete_valuation : concrete_valuations_at_k.second) {
            std::stringstream pretty_string;
            pretty_string << fmt::format(fmt::fg(fmt::terminal_color::green), concrete_valuation.first) << " -> "
                          << concrete_valuation.second;
            unsigned int cycle = extract_cycle(concrete_valuation.first);
            auto it = pretty_printing_concrete_expressions.find(cycle);
            if (it == pretty_printing_concrete_expressions.end()) {
                pretty_printing_concrete_expressions.emplace(
                        cycle, std::map<unsigned int, std::vector<std::string>>{{k, {pretty_string.str()}}});
            } else {
                auto nested_it = it->second.find(k);
                if (nested_it == it->second.end()) {
                    it->second.emplace(k, std::vector<std::string>{pretty_string.str()});
                } else {
                    it->second.at(k).push_back(pretty_string.str());
                }
            }
        }
    }
    for (auto it = pretty_printing_concrete_expressions.begin(); it != pretty_printing_concrete_expressions.end();
         ++it) {
        str << "\t\t\t"
            << fmt::format(fmt::fg(fmt::terminal_color::black) | fmt::emphasis::bold,
                           "cycle " + std::to_string(it->first))
            << ": [\n";
        for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
            str << "\t\t\t\tk = " << it1->first << ": ";
            str << "{";
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
                str << *it2;
                if (std::next(it2) != it1->second.end()) {
                    str << ", ";
                }
            }
            str << "}";
            if (std::next(it1) != it->second.end()) {
                str << "\n";
            }
        }
        str << "\n\t\t\t]";
        if (std::next(it) != pretty_printing_concrete_expressions.end()) {
            str << ",\n ";
        }
    }
    str << "\n\t\t},\n";
    str << "\t\tsymbolic valuations: {\n";
    std::map<unsigned int, std::map<unsigned int, std::vector<std::string>>> pretty_printing_symbolic_expressions;
    for (const auto &symbolic_valuations_at_k : _symbolic_valuations) {
        unsigned int k = symbolic_valuations_at_k.first;
        for (const auto &symbolic_valuation : symbolic_valuations_at_k.second) {
            std::stringstream pretty_string;
            pretty_string << fmt::format(fmt::fg(fmt::terminal_color::green), symbolic_valuation.first) << " -> "
                          << symbolic_valuation.second;
            unsigned int cycle = extract_cycle(symbolic_valuation.first);
            auto it = pretty_printing_symbolic_expressions.find(cycle);
            if (it == pretty_printing_symbolic_expressions.end()) {
                pretty_printing_symbolic_expressions.emplace(
                        cycle, std::map<unsigned int, std::vector<std::string>>{{k, {pretty_string.str()}}});
            } else {
                auto nested_it = it->second.find(k);
                if (nested_it == it->second.end()) {
                    it->second.emplace(k, std::vector<std::string>{pretty_string.str()});
                } else {
                    it->second.at(k).push_back(pretty_string.str());
                }
            }
        }
    }
    for (auto it = pretty_printing_symbolic_expressions.begin(); it != pretty_printing_symbolic_expressions.end();
         ++it) {
        str << "\t\t\t"
            << fmt::format(fmt::fg(fmt::terminal_color::black) | fmt::emphasis::bold,
                           "cycle " + std::to_string(it->first))
            << ": [\n";
        for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1) {
            str << "\t\t\t\tk = " << it1->first << ": ";
            str << "{";
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
                str << *it2;
                if (std::next(it2) != it1->second.end()) {
                    str << ", ";
                }
            }
            str << "}";
            if (std::next(it1) != it->second.end()) {
                str << "\n";
            }
        }
        str << "\n\t\t\t]";
        if (std::next(it) != pretty_printing_symbolic_expressions.end()) {
            str << ",\n ";
        }
    }
    str << "\n\t\t},\n";
    str << "\t\tpath constraint: ";
    str << "[";
    for (auto constraint = _path_constraint.begin(); constraint != _path_constraint.end(); ++constraint) {
        str << *constraint;
        if (std::next(constraint) != _path_constraint.end()) {
            str << ", ";
        }
    }
    str << "]\n";
    str << "\t)";
    return os << str.str();
}

const Vertex &State::getVertex() const {
    return *_vertex;
}

void State::setVertex(const Vertex &vertex) {
    _vertex = &vertex;
}

z3::expr State::getConcreteValuation(const std::string &contextualized_name) const {
    auto it = std::find_if(_concrete_valuations.begin(), _concrete_valuations.end(),
                           [contextualized_name](const auto &concrete_valuations_at_k) {
                               for (const auto &concrete_valuation : concrete_valuations_at_k.second) {
                                   if (concrete_valuation.first == contextualized_name) {
                                       return true;
                                   }
                               }
                               return false;
                           });
    assert(it != _concrete_valuations.end());
    return it->second.at(contextualized_name);
}

void State::setConcreteValuation(unsigned int k, const std::string &contextualized_name, const z3::expr &valuation) {
    auto it = std::find_if(_concrete_valuations.begin(), _concrete_valuations.end(),
                           [k](const auto &concrete_valuations_at_k) { return concrete_valuations_at_k.first == k; });
    if (it == _concrete_valuations.end()) {
        _concrete_valuations.emplace(k, std::map<std::string, z3::expr>{{contextualized_name, valuation}});
    } else {
        assert(_concrete_valuations.at(k).find(contextualized_name) == _concrete_valuations.at(k).end());
        _concrete_valuations.at(k).emplace(contextualized_name, valuation);
    }
}

z3::expr State::getSymbolicValuation(const std::string &contextualized_name) const {
    auto it = std::find_if(_symbolic_valuations.begin(), _symbolic_valuations.end(),
                           [contextualized_name](const auto &symbolic_valuations_at_k) {
                               for (const auto &symbolic_valuation : symbolic_valuations_at_k.second) {
                                   if (symbolic_valuation.first == contextualized_name) {
                                       return true;
                                   }
                               }
                               return false;
                           });
    assert(it != _symbolic_valuations.end());
    return it->second.at(contextualized_name);
}

void State::setSymbolicValuation(unsigned int k, const std::string &contextualized_name, const z3::expr &valuation) {
    auto it = std::find_if(_symbolic_valuations.begin(), _symbolic_valuations.end(),
                           [k](const auto &symbolic_valuations_at_k) { return symbolic_valuations_at_k.first == k; });
    if (it == _symbolic_valuations.end()) {
        _symbolic_valuations.emplace(k, std::map<std::string, z3::expr>{{contextualized_name, valuation}});
    } else {
        assert(_symbolic_valuations.at(k).find(contextualized_name) == _symbolic_valuations.at(k).end());
        _symbolic_valuations.at(k).emplace(contextualized_name, valuation);
    }
}

const State::store_t &State::getSymbolicValuations() const {
    return _symbolic_valuations;
}

const std::vector<z3::expr> &State::getPathConstraint() const {
    return _path_constraint;
}

void State::pushPathConstraint(const z3::expr &constraint) {
    _path_constraint.push_back(constraint);
}

void State::backtrack(const Vertex &vertex, int j, z3::model &model, std::vector<z3::expr> path_constraint) {
    _vertex = &vertex;
    _path_constraint = std::move(path_constraint);
    store_t concrete_valuations;
    for (int i = 0; i <= j; ++i) {
        concrete_valuations.emplace(i, _concrete_valuations.at(i));
    }
    _concrete_valuations = concrete_valuations;
    for (unsigned int i = 0; i < model.size(); ++i) {
        z3::func_decl constant_declaration = model.get_const_decl(i);
        assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
        std::string contextualized_name = constant_declaration.name().str();
        z3::expr interpretation = model.get_const_interp(constant_declaration);
        for (auto &concrete_valuation_at_k : _concrete_valuations) {
            for (auto &concrete_valuation : concrete_valuation_at_k.second) {
                if (concrete_valuation.first == contextualized_name) {
                    concrete_valuation.second = interpretation;
                    break;
                }
            }
        }
    }
    store_t symbolic_valuations;
    for (int i = 0; i <= j; ++i) {
        symbolic_valuations.emplace(i, _symbolic_valuations.at(i));
    }
    _symbolic_valuations = symbolic_valuations;
}

unsigned int State::getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                      bool from_concrete_valuations) const {
    if (from_concrete_valuations) {
        return getHighestVersion(flattened_name, cycle, _concrete_valuations);
    } else {
        return getHighestVersion(flattened_name, cycle, _symbolic_valuations);
    }
}

unsigned int State::getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                      const store_t &valuations) const {
    unsigned int highest_version = 0;
    for (const auto &valuations_at_k : valuations) {
        for (const auto &valuation : valuations_at_k.second) {
            std::string contextualized_name = valuation.first;
            std::size_t version_position = contextualized_name.find('_');
            std::size_t cycle_position = contextualized_name.find("__");
            if (flattened_name == contextualized_name.substr(0, version_position)) {
                if (cycle == std::stoul(contextualized_name.substr(cycle_position + 2, contextualized_name.size()))) {
                    unsigned int version = std::stoul(
                            contextualized_name.substr(version_position + 1, cycle_position - version_position - 1));
                    if (highest_version < version) {
                        highest_version = version;
                    }
                }
            }
        }
    }
    return highest_version;
}