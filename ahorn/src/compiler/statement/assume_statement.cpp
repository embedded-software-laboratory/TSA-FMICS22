#include "compiler/statement/assume_statement.h"

using namespace ir;

AssumeStatement::AssumeStatement(std::unique_ptr<Expression> expression) : _expression(std::move(expression)) {}

void AssumeStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

std::ostream &AssumeStatement::print(std::ostream &os) const {
    return os << "ASSUME";
}

AssumeStatement *AssumeStatement::clone_implementation() const {
    std::unique_ptr<Expression> expression = _expression->clone();
    return new AssumeStatement(std::move(expression));
}

Expression &AssumeStatement::getExpression() const {
    return *_expression;
}
