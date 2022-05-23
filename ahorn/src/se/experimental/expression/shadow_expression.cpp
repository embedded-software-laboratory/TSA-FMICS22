#include "se/experimental/expression/shadow_expression.h"

#include <sstream>

using namespace se;

ShadowExpression::ShadowExpression(z3::expr z3_expression, std::shared_ptr<Expression> old_expression,
                                   std::shared_ptr<Expression> new_expression)
    : Expression(Kind::SHADOW_EXPRESSION, std::move(z3_expression)), _old_expression(std::move(old_expression)),
      _new_expression(std::move(new_expression)) {}

std::ostream &ShadowExpression::print(std::ostream &os) const {
    std::stringstream str;
    str << "<" << *_old_expression << ", " << *_new_expression << ">";
    return os << str.str();
}

std::shared_ptr<Expression> ShadowExpression::getOldExpression() const {
    return _old_expression;
}

std::shared_ptr<Expression> ShadowExpression::getNewExpression() const {
    return _new_expression;
}