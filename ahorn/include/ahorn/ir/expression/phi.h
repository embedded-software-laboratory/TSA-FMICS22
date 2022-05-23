#ifndef AHORN_PHI_H
#define AHORN_PHI_H

#include "ir/expression/expression.h"
#include "ir/expression/variable_access.h"
#include "ir/variable.h"

#include <boost/iterator/indirect_iterator.hpp>

#include <map>
#include <memory>

namespace ir {
    class Phi : public Expression {
    public:
        Phi(std::shared_ptr<Variable> variable,
            std::map<unsigned int, std::unique_ptr<VariableAccess>> label_to_operand);
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::shared_ptr<Variable> getVariable() const;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<Phi> clone() const {
            return std::unique_ptr<Phi>(this->clone_implementation());
        }

        const std::map<unsigned int, std::unique_ptr<VariableAccess>> &getLabelToOperand() const;

    private:
        Phi *clone_implementation() const override;

    private:
        // the target of the PHI expression
        std::shared_ptr<Variable> _variable;
        std::map<unsigned int, std::unique_ptr<VariableAccess>> _label_to_operand;
    };
}// namespace ir

#endif//AHORN_PHI_H
