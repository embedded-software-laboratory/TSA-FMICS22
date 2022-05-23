#include "ir/expression/undefined.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"
#include "ir/type/inconclusive_type.h"

using namespace ir;

Undefined::Undefined() : Expression(Expression::Kind::UNDEFINED) {
    _type = Expression::Type::UNDEFINED;
}

void Undefined::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &Undefined::print(std::ostream &os) const {
    return os << "?";
}

Undefined *Undefined::clone_implementation() const {
    return new Undefined();
}

void Undefined::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
