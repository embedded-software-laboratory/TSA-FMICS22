#ifndef AHORN_STATEMENT_H
#define AHORN_STATEMENT_H

#include "statement_visitor.h"

#include <iostream>
#include <memory>

class Statement {
public:
    virtual ~Statement() = default;
    virtual void accept(StatementVisitor &visitor) = 0;
    virtual std::ostream &print(std::ostream &os) const = 0;
    friend std::ostream &operator<<(std::ostream &os, const Statement &statement) { return statement.print(os); }
    std::unique_ptr<Statement> clone() const { return std::unique_ptr<Statement>(this->clone_implementation()); }

private:
    virtual Statement *clone_implementation() const = 0;
};

#endif//AHORN_STATEMENT_H
