#ifndef AHORN_ASSIGNMENT_STATEMENT_H
#define AHORN_ASSIGNMENT_STATEMENT_H

#include "ir/expression/expression.h"
#include "ir/expression/variable_reference.h"
#include "statement.h"

#include <memory>

class AssignmentStatement : public Statement {
public:
    AssignmentStatement(std::unique_ptr<ir::VariableReference> variable_reference,
                        std::unique_ptr<ir::Expression> expression);

    const ir::VariableReference &getVariableReference() const;

    ir::Expression &getExpression() const;

    void setExpression(std::unique_ptr<ir::Expression> expression);

    void accept(StatementVisitor &visitor) override;

    std::ostream &print(std::ostream &os) const override;
    std::unique_ptr<AssignmentStatement> clone() const {
        return std::unique_ptr<AssignmentStatement>(static_cast<AssignmentStatement *>(this->clone_implementation()));
    }

private:
    AssignmentStatement *clone_implementation() const override;
    const std::unique_ptr<ir::VariableReference> _variable_reference;
    std::unique_ptr<ir::Expression> _expression;
};

#endif//AHORN_ASSIGNMENT_STATEMENT_H
