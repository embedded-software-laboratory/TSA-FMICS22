#ifndef AHORN_ASSERT_STATEMENT_H
#define AHORN_ASSERT_STATEMENT_H

#include "ir/expression/expression.h"
#include "statement.h"

#include <memory>

class AssertStatement : public Statement {
public:
    AssertStatement(std::unique_ptr<ir::Expression> expression);

    ir::Expression &getExpression() const;

    void accept(StatementVisitor &visitor) override;

    std::ostream &print(std::ostream &os) const override;

    std::unique_ptr<AssertStatement> clone() const {
        return std::unique_ptr<AssertStatement>(static_cast<AssertStatement *>(this->clone_implementation()));
    }

private:
    AssertStatement *clone_implementation() const override;

private:
    const std::unique_ptr<ir::Expression> _expression;
};

#endif//AHORN_ASSERT_STATEMENT_H
