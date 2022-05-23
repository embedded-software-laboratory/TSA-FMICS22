#ifndef AHORN_ASSIGNMENT_INSTRUCTION_H
#define AHORN_ASSIGNMENT_INSTRUCTION_H

#include "ir/expression/expression.h"
#include "ir/expression/variable_reference.h"
#include "ir/instruction/instruction.h"

#include <memory>

namespace ir {
    // an assignment instruction can either be a regular assignment, e.g., assigning to a primitive variable or a
    // structured variable (Type::REGULAR), or it can assign to function call input parameters and output paramaters
    // (Type::PARAMATER_IN and Type::PARAMETER_OUT, respectively).
    class AssignmentInstruction : public Instruction {
    public:
        enum class Type { REGULAR = 1, PARAMETER_IN = 2, PARAMETER_OUT = 3 };

        AssignmentInstruction(std::unique_ptr<VariableReference> variable_reference,
                              std::unique_ptr<Expression> expression, Type type = Type::REGULAR);
        const VariableReference &getVariableReference() const;
        const Expression &getExpression() const;

        void accept(InstructionVisitor &visitor) const override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<AssignmentInstruction> clone() const {
            return std::unique_ptr<AssignmentInstruction>(
                    static_cast<AssignmentInstruction *>(this->clone_implementation()));
        }

        void setType(Type type);
        Type getType() const;

    private:
        AssignmentInstruction *clone_implementation() const override;

    private:
        const std::unique_ptr<VariableReference> _variable_reference;
        const std::unique_ptr<Expression> _expression;
        Type _type;
    };
}// namespace ir

#endif//AHORN_ASSIGNMENT_INSTRUCTION_H
