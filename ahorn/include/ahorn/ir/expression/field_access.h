#ifndef AHORN_FIELD_ACCESS_H
#define AHORN_FIELD_ACCESS_H

#include "ir/expression/variable_access.h"
#include "ir/expression/variable_reference.h"

namespace ir {
    class FieldAccess : public VariableReference {
    public:
        FieldAccess(std::unique_ptr<VariableAccess> record_access,
                    std::unique_ptr<VariableReference> variable_reference);
        const VariableAccess &getRecordAccess() const;
        const VariableReference &getVariableReference() const;
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<FieldAccess> clone() const {
            return std::unique_ptr<FieldAccess>(static_cast<FieldAccess *>(this->clone_implementation()));
        }

        // retrieves the variable access expression belonging to the accessed field
        const VariableAccess &getVariableAccess() const;

        std::string getName() const override;

    private:
        FieldAccess *clone_implementation() const override;

    private:
        std::unique_ptr<VariableAccess> _record_access;
        // access to contained variable, can again be a container
        std::unique_ptr<VariableReference> _variable_reference;
    };
}// namespace ir

#endif//AHORN_FIELD_ACCESS_H
