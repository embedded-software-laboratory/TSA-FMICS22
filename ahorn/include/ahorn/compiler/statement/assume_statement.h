#ifndef AHORN_ASSUME_STATEMENT_H
#define AHORN_ASSUME_STATEMENT_H

#include "ir/expression/expression.h"
#include "statement.h"

#include <memory>

class AssumeStatement : public Statement {
public:
    explicit AssumeStatement(std::unique_ptr<ir::Expression> expression);

    ir::Expression &getExpression() const;

    void accept(StatementVisitor &visitor) override;

    std::ostream &print(std::ostream &os) const override;

    std::unique_ptr<AssumeStatement> clone() const {
        return std::unique_ptr<AssumeStatement>(static_cast<AssumeStatement *>(this->clone_implementation()));
    }

private:
    AssumeStatement *clone_implementation() const override;

private:
    const std::unique_ptr<ir::Expression> _expression;
};

#endif//AHORN_ASSUME_STATEMENT_H
