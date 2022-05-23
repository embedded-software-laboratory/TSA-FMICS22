#include "se/baseline/context/state.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::baseline;

State::State(const Vertex &vertex, store_t concrete_valuations, store_t symbolic_valuations,
             std::vector<z3::expr> path_constraint)
    : _vertex(&vertex), _concrete_valuations(std::move(concrete_valuations)),
      _symbolic_valuations(std::move(symbolic_valuations)), _path_constraint(std::move(path_constraint)) {}

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
    std::map<unsigned int, std::vector<std::string>> pretty_printing_concrete_expressions;
    for (const auto &p : _concrete_valuations) {
        std::stringstream pretty_string;
        pretty_string << fmt::format(fmt::fg(fmt::terminal_color::green), p.first) << " -> " << p.second;
        unsigned int cycle = extract_cycle(p.first);
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
    str << "\t\tsymbolic valuations: {\n";
    std::map<unsigned int, std::vector<std::string>> pretty_printing_symbolic_expressions;
    for (const auto &p : _symbolic_valuations) {
        std::stringstream pretty_string;
        pretty_string << fmt::format(fmt::fg(fmt::terminal_color::blue), p.first) << " -> " << p.second;
        unsigned int cycle = extract_cycle(p.first);
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
    assert(_concrete_valuations.find(contextualized_name) != _concrete_valuations.end());
    return _concrete_valuations.at(contextualized_name);
}

void State::setConcreteValuation(const std::string &contextualized_name, const z3::expr &valuation) {
    assert(_concrete_valuations.find(contextualized_name) == _concrete_valuations.end());
    _concrete_valuations.emplace(contextualized_name, valuation);
}

const State::store_t &State::getSymbolicValuations() const {
    return _symbolic_valuations;
}

z3::expr State::getSymbolicValuation(const std::string &contextualized_name) const {
    assert(_symbolic_valuations.find(contextualized_name) != _symbolic_valuations.end());
    return _symbolic_valuations.at(contextualized_name);
}

void State::setSymbolicValuation(const std::string &contextualized_name, const z3::expr &valuation) {
    assert(_symbolic_valuations.find(contextualized_name) == _symbolic_valuations.end());
    _symbolic_valuations.emplace(contextualized_name, valuation);
}

const std::vector<z3::expr> &State::getPathConstraint() const {
    return _path_constraint;
}

void State::pushPathConstraint(const z3::expr &expression) {
    _path_constraint.push_back(expression);
}

std::unique_ptr<State> State::fork(Solver &solver, const Vertex &vertex, z3::model &model,
                                   const z3::expr &expression) const {
    auto logger = spdlog::get("Baseline");
    std::vector<std::string> printable_concrete_valuations;
    // XXX copy concrete valuations and update valuations from the model, it is not necessarily complete, hence reuse
    // XXX old valuations that were irrelevant for the satisfiability of the fork, i.e., "don't care"
    store_t concrete_valuations = _concrete_valuations;
    for (unsigned int i = 0; i < model.size(); ++i) {
        z3::func_decl constant_declaration = model.get_const_decl(i);
        assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
        std::string contextualized_name = constant_declaration.name().str();
        z3::expr interpretation = model.get_const_interp(constant_declaration);
        concrete_valuations.at(contextualized_name) = interpretation;
        printable_concrete_valuations.push_back(contextualized_name + " -> " + interpretation.to_string());
    }
    SPDLOG_LOGGER_TRACE(logger, "Updating forked state with concrete valuations [{}] from model.",
                        fmt::join(printable_concrete_valuations, ", "));
    std::vector<z3::expr> path_constraint{_path_constraint};
    path_constraint.push_back(expression);
    return std::make_unique<State>(vertex, std::move(concrete_valuations), _symbolic_valuations,
                                   std::move(path_constraint));
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
    for (const auto &valuation : valuations) {
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
    return highest_version;
}