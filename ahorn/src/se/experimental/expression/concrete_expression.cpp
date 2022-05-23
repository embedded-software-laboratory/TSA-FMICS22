#include "se/experimental/expression/concrete_expression.h"

using namespace se;

ConcreteExpression::ConcreteExpression(z3::expr z3_expression)
    : Expression(Kind::CONCRETE_EXPRESSION, std::move(z3_expression)) {}

std::ostream &ConcreteExpression::print(std::ostream &os) const {
    return os << getZ3Expression().to_string();
}