#ifndef AHORN_CASE_STATEMENT_H
#define AHORN_CASE_STATEMENT_H

#include "compiler/statement/selection_statement.h"
#include "ir/expression/expression.h"

#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

using ElseIfStatement = std::pair<std::unique_ptr<ir::Expression>, std::vector<std::unique_ptr<Statement>>>;

class CaseStatement : public SelectionStatement {
public:
    CaseStatement(std::unique_ptr<ir::Expression> expression, std::vector<ElseIfStatement> cases,
                  std::vector<std::unique_ptr<Statement>> else_statements);
    const ir::Expression &getExpression() const;
    const std::vector<ElseIfStatement> &getCases() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> elseStatementsBegin() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator> elseStatementsEnd() const;
    void accept(StatementVisitor &visitor) override;
    std::ostream &print(std::ostream &os) const override;
    std::unique_ptr<CaseStatement> clone() const {
        return std::unique_ptr<CaseStatement>(static_cast<CaseStatement *>(this->clone_implementation()));
    }

private:
    CaseStatement *clone_implementation() const override;

private:
    const std::unique_ptr<ir::Expression> _expression;
    const std::vector<ElseIfStatement> _cases;
    const std::vector<std::unique_ptr<Statement>> _else_statements;
};

#endif//AHORN_CASE_STATEMENT_H
