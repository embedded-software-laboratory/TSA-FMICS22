#include "se/experimental/test_case.h"

#include <sstream>

using namespace se;

TestCase::TestCase(std::map<unsigned int, std::map<std::string, z3::expr>> inputs,
                   std::map<unsigned int, std::map<std::string, std::shared_ptr<ConcreteExpression>>> outputs)
    : _inputs(std::move(inputs)), _outputs(std::move(outputs)) {}

std::ostream &TestCase::print(std::ostream &os) const {
    std::stringstream str;
    str << "{\n";
    for (auto it = _inputs.begin(); it != _inputs.end(); ++it) {
        unsigned int cycle = it->first;
        str << "\tcycle: " << cycle << "\n";
        str << "\t\tinputs: [";
        for (auto inputs = it->second.begin(); inputs != it->second.end(); ++inputs) {
            str << inputs->first << " -> " << inputs->second;
            if (std::next(inputs) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]\n";
        if (std::next(it) != _inputs.end()) {
            str << "\n ";
        }
    }
    for (auto it = _outputs.begin(); it != _outputs.end(); ++it) {
        unsigned int cycle = it->first;
        str << "\tcycle: " << cycle << "\n";
        str << "\t\tresponse: [";
        for (auto response = it->second.begin(); response != it->second.end(); ++response) {
            str << response->first << " -> " << *response->second;
            if (std::next(response) != it->second.end()) {
                str << ", ";
            }
        }
        str << "]\n";
        if (std::next(it) != _outputs.end()) {
            str << "\n ";
        }
    }
    str << "}";
    return os << str.str();
}

const std::map<unsigned int, std::map<std::string, z3::expr>> &TestCase::getInputs() const {
    return _inputs;
}