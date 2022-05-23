#ifndef AHORN_IF_STATEMENT_H
#define AHORN_IF_STATEMENT_H

#include "ir/expression/expression.h"
#include "selection_statement.h"
#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

using ElseIfStatement = std::pair<std::unique_ptr<ir::Expression>, std::vector<std::unique_ptr<Statement>>>;

class IfStatement : public SelectionStatement {
public:
    IfStatement(std::unique_ptr<ir::Expression> expression, std::vector<std::unique_ptr<Statement>> then_statements,
                std::vector<ElseIfStatement> else_if_statements,
                std::vector<std::unique_ptr<Statement>> else_statements);

    ir::Expression &getExpression() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> thenStatementsBegin() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> thenStatementsEnd() const;
    const std::vector<ElseIfStatement> &getElseIfStatements() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> elseStatementsBegin() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> elseStatementsEnd() const;
    void accept(StatementVisitor &visitor) override;

    bool hasThenStatements() const;
    bool hasElseIfStatements() const;
    bool hasElseStatements() const;

    std::ostream &print(std::ostream &os) const override;
    std::unique_ptr<IfStatement> clone() const {
        return std::unique_ptr<IfStatement>(static_cast<IfStatement *>(this->clone_implementation()));
    }

private:
    IfStatement *clone_implementation() const override;

private:
    const std::unique_ptr<ir::Expression> _expression;
    const std::vector<std::unique_ptr<Statement>> _then_statements;
    const std::vector<ElseIfStatement> _else_if_statements;
    const std::vector<std::unique_ptr<Statement>> _else_statements;
};

#endif//AHORN_IF_STATEMENT_H
