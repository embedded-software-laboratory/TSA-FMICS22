#ifndef AHORN_WHILE_STATEMENT_H
#define AHORN_WHILE_STATEMENT_H

#include "ir/expression/expression.h"
#include "iteration_statement.h"
#include "statement.h"
#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

class WhileStatement : public IterationStatement {
private:
    using const_statement_it = boost::indirect_iterator<std::vector<std::unique_ptr<Statement>>::const_iterator>;

public:
    WhileStatement(std::unique_ptr<ir::Expression> expression, std::vector<std::unique_ptr<Statement>> statements);

    const_statement_it statementsBegin() const;
    const_statement_it statementsEnd() const;
    ir::Expression &getExpression() const;
    void accept(StatementVisitor &visitor) override;
    bool hasStatements() const;

    std::ostream &print(std::ostream &os) const override;
    std::unique_ptr<WhileStatement> clone() const {
        return std::unique_ptr<WhileStatement>(static_cast<WhileStatement *>(this->clone_implementation()));
    }

private:
    WhileStatement *clone_implementation() const override;
    std::unique_ptr<ir::Expression> _expression;
    std::vector<std::unique_ptr<Statement>> _statements;
};

#endif//AHORN_WHILE_STATEMENT_H
