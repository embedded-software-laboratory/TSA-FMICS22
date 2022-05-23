#include "se/oa/context/state.h"
#include "se/utilities/fmt_formatter.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::oa;

State::State(const Vertex &vertex, std::map<std::string, z3::expr> symbolic_valuations,
             std::vector<z3::expr> path_constraint, std::map<std::string, unsigned int> flattened_name_to_version)
    : _vertex(&vertex), _symbolic_valuations(std::move(symbolic_valuations)),
      _path_constraint(std::move(path_constraint)), _flattened_name_to_version(std::move(flattened_name_to_version)) {}

std::ostream &State::print(std::ostream &os) const {
    auto extract_cycle = [](const std::string &contextualized_name) {
        std::size_t cycle_position = contextualized_name.find("__");
        unsigned int cycle = std::stoi(contextualized_name.substr(cycle_position + 2, contextualized_name.size()));
        return cycle;
    };
    std::stringstream str;
    str << "(\n";
    str << "\t\tvertex: " << *_vertex << ",\n";
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

const std::map<std::string, z3::expr> &State::getSymbolicValuations() const {
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

void State::setVersion(const std::string &flattened_name, unsigned int version) {
    assert(_flattened_name_to_version.find(flattened_name) != _flattened_name_to_version.end());
    _flattened_name_to_version.at(flattened_name) = version;
}

std::unique_ptr<State> State::fork(Solver &solver, const Vertex &vertex, z3::model &model,
                                   const z3::expr &expression) const {
    auto logger = spdlog::get("Oa");
    std::vector<z3::expr> path_constraint{_path_constraint};
    path_constraint.push_back(expression);
    return std::make_unique<State>(vertex, _symbolic_valuations, std::move(path_constraint),
                                   _flattened_name_to_version);
}

unsigned int State::getHighestVersion(const std::string &flattened_name) const {
    return _flattened_name_to_version.at(flattened_name);
}