#include "se/experimental/symbolic_variable.h"

#include <sstream>

using namespace se;

SymbolicVariable::SymbolicVariable(std::string flattened_name, unsigned int version, unsigned int cycle)
    : _flattened_name(std::move(flattened_name)), _version(version), _cycle(cycle) {}

std::ostream &SymbolicVariable::print(std::ostream &os) const {
    std::stringstream str;
    throw std::logic_error("Not implemented yet.");
    return os << str.str();
}