#include "se/experimental/expression/assumption_literal_expression.h"

#include <sstream>

using namespace se;

AssumptionLiteralExpression::AssumptionLiteralExpression(z3::expr z3_expression)
    : Expression(Kind::ASSUMPTION_LITERAL_EXPRESSION, std::move(z3_expression)) {}

std::ostream &AssumptionLiteralExpression::print(std::ostream &os) const {
    std::stringstream str;
    str << getZ3Expression().to_string();
    return os << str.str();
}
