#include "se/experimental/expression/symbolic_expression.h"

#include <stdexcept>

using namespace se;

SymbolicExpression::SymbolicExpression(z3::expr z3_expression)
    : Expression(Kind::SYMBOLIC_EXPRESSION, std::move(z3_expression)) {}

std::ostream &SymbolicExpression::print(std::ostream &os) const {
    return os << getZ3Expression().to_string();
}