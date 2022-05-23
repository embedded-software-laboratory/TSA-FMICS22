#include "compiler/statement/case_statement.h"
#include "compiler/statement/statement_visitor.h"

using namespace ir;

CaseStatement::CaseStatement(std::unique_ptr<Expression> expression, std::vector<ElseIfStatement> cases,
                             std::vector<std::unique_ptr<Statement>> else_statements)
    : _expression(std::move(expression)), _cases(std::move(cases)), _else_statements(std::move(else_statements)) {}

const Expression &CaseStatement::getExpression() const {
    return *_expression;
}

const std::vector<ElseIfStatement> &CaseStatement::getCases() const {
    return _cases;
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
CaseStatement::elseStatementsBegin() const {
    return std::begin(_else_statements);
}

boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>
CaseStatement::elseStatementsEnd() const {
    return std::end(_else_statements);
}

void CaseStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

std::ostream &CaseStatement::print(std::ostream &os) const {
    throw std::logic_error("Not implemented yet.");
}

CaseStatement *CaseStatement::clone_implementation() const {
    std::unique_ptr<Expression> expression = _expression->clone();
    std::vector<ElseIfStatement> cases;
    for (const auto &case_ : _cases) {
        std::vector<std::unique_ptr<Statement>> statements;
        for (auto &statement : case_.second) {
            statements.push_back(statement->clone());
        }
        cases.emplace_back(case_.first->clone(), std::move(statements));
    }
    std::vector<std::unique_ptr<Statement>> else_statements;
    for (const auto &else_statement : _else_statements) {
        else_statements.push_back(else_statement->clone());
    }
    return new CaseStatement(std::move(expression), std::move(cases), std::move(else_statements));
}
