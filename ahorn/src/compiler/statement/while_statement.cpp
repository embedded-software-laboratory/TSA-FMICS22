#include "compiler/statement/while_statement.h"

using namespace ir;

WhileStatement::WhileStatement(std::unique_ptr<Expression> expression,
                               std::vector<std::unique_ptr<Statement>> statements)
    : _expression(std::move(expression)), _statements(std::move(statements)) {}

std::ostream &WhileStatement::print(std::ostream &os) const {
    return os << "WhileStatement" << std::endl;
}

void WhileStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

Expression &WhileStatement::getExpression() const {
    return *_expression;
}

WhileStatement::const_statement_it WhileStatement::statementsBegin() const {
    return std::begin(_statements);
}

WhileStatement::const_statement_it WhileStatement::statementsEnd() const {
    return std::end(_statements);
}

WhileStatement *WhileStatement::clone_implementation() const {
    std::unique_ptr<Expression> expression = _expression->clone();
    std::vector<std::unique_ptr<Statement>> statements;
    for (const auto &statement : _statements) {
        statements.emplace_back(std::move(statement->clone()));
    }
    return new WhileStatement(std::move(expression), std::move(statements));
}

bool WhileStatement::hasStatements() const {
    return !_statements.empty();
}
