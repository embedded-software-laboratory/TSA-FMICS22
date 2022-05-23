#include "se/experimental/expression/expression.h"

using namespace se;

unsigned int Expression::_next_id = 0;

Expression::Expression(Kind kind, z3::expr z3_expression)
    : _id(_next_id++), _kind(kind), _z3_expression(std::move(z3_expression)) {}

unsigned int Expression::getId() const {
    return _id;
}

Expression::Kind Expression::getKind() const {
    return _kind;
}

const z3::expr &Expression::getZ3Expression() const {
    return _z3_expression;
}