#include "compiler/statement/if_statement.h"
#include <algorithm>

#include <sstream>

using namespace ir;

IfStatement::IfStatement(std::unique_ptr<Expression> expression,
                         std::vector<std::unique_ptr<Statement>> then_statements,
                         std::vector<ElseIfStatement> else_if_statements,
                         std::vector<std::unique_ptr<Statement>> else_statements)
    : _expression(std::move(expression)), _then_statements(std::move(then_statements)),
      _else_if_statements(std::move(else_if_statements)), _else_statements(std::move(else_statements)) {}

std::ostream &IfStatement::print(std::ostream &os) const {
    std::stringstream str;
    str << "if " << *_expression << " then \n";
    for (auto &then_statement : _then_statements) {
        str << *then_statement << "\n";
    }
    for (auto &else_if_statement : _else_if_statements) {
        str << "else if " << *else_if_statement.first << "\n";
        for (auto &statement : else_if_statement.second) {
            str << *statement << "\n";
        }
    }
    if (!_else_statements.empty()) {
        str << " else \n";
    }
    for (auto &else_statement : _else_statements) {
        str << *else_statement << "\n";
    }
    return os << str.str();
}

void IfStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

Expression &IfStatement::getExpression() const {
    return *_expression;
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
IfStatement::thenStatementsBegin() const {
    return std::begin(_then_statements);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
IfStatement::thenStatementsEnd() const {
    return std::end(_then_statements);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
IfStatement::elseStatementsBegin() const {
    return std::begin(_else_statements);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
IfStatement::elseStatementsEnd() const {
    return std::end(_else_statements);
}

IfStatement *IfStatement::clone_implementation() const {
    std::unique_ptr<Expression> expression = _expression->clone();
    std::vector<std::unique_ptr<Statement>> then_statements;
    for (const auto &then_statement : _then_statements) {
        then_statements.push_back(then_statement->clone());
    }
    std::vector<ElseIfStatement> else_if_statements;
    for (const auto &else_if_statement : _else_if_statements) {
        std::vector<std::unique_ptr<Statement>> statements;
        for (auto &statement : else_if_statement.second) {
            statements.push_back(statement->clone());
        }
        else_if_statements.emplace_back(else_if_statement.first->clone(), std::move(statements));
    }
    std::vector<std::unique_ptr<Statement>> else_statements;
    for (const auto &else_statement : _else_statements) {
        else_statements.push_back(else_statement->clone());
    }
    return new IfStatement(std::move(expression), std::move(then_statements), std::move(else_if_statements),
                           std::move(else_statements));
}

bool IfStatement::hasThenStatements() const {
    return !_then_statements.empty();
}

bool IfStatement::hasElseIfStatements() const {
    return !_else_if_statements.empty();
}

bool IfStatement::hasElseStatements() const {
    return !_else_statements.empty();
}

const std::vector<ElseIfStatement> &IfStatement::getElseIfStatements() const {
    return _else_if_statements;
}
