#include "ir/expression/constant/nondeterministic_constant.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

NondeterministicConstant::NondeterministicConstant(Expression::Type type) : Constant(Kind::NONDETERMINISTIC_CONSTANT) {
    _type = type;
}

void NondeterministicConstant::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &NondeterministicConstant::print(std::ostream &os) const {
    os << "TOP";
    return os;
}

NondeterministicConstant *NondeterministicConstant::clone_implementation() const {
    return new NondeterministicConstant(getType());
}

void NondeterministicConstant::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
