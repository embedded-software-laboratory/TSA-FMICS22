#include "se/simulator/context/state.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::simulator;

State::State(const Vertex &vertex, std::map<std::string, z3::expr> concrete_valuations)
    : _vertex(&vertex), _concrete_valuations(std::move(concrete_valuations)) {}

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
    str << "\n\t\t}\n";
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

void State::updateConcreteValuation(const std::string &contextualized_name, const z3::expr &valuation) {
    auto it = _concrete_valuations.find(contextualized_name);
    if (it == _concrete_valuations.end()) {
        throw std::runtime_error("Concrete valuation not found.");
    } else {
        it->second = valuation;
    }
}