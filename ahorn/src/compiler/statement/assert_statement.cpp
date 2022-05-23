#include "compiler/statement/assert_statement.h"

using namespace ir;

AssertStatement::AssertStatement(std::unique_ptr<Expression> expression) : _expression(std::move(expression)) {}

void AssertStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

std::ostream &AssertStatement::print(std::ostream &os) const {
    return os << "ASSERT";
}

AssertStatement *AssertStatement::clone_implementation() const {
    std::unique_ptr<Expression> expression = _expression->clone();
    return new AssertStatement(std::move(expression));
}

Expression &AssertStatement::getExpression() const {
    return *_expression;
}
