#ifndef AHORN_VARIABLE_ACCESS_H
#define AHORN_VARIABLE_ACCESS_H

#include "ir/expression/variable_reference.h"
#include "ir/variable.h"

#include <memory>
#include <string>

namespace ir {
    class VariableAccess : public VariableReference {
    public:
        explicit VariableAccess(std::shared_ptr<Variable> variable);
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<VariableAccess> clone() const {
            return std::unique_ptr<VariableAccess>(static_cast<VariableAccess *>(this->clone_implementation()));
        }

        std::string getName() const override;

        std::shared_ptr<Variable> getVariable() const;

    private:
        VariableAccess *clone_implementation() const override;

    private:
        std::shared_ptr<Variable> _variable;
    };
}// namespace ir

#endif//AHORN_VARIABLE_ACCESS_H
