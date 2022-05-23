#ifndef AHORN_INSTRUCTION_H
#define AHORN_INSTRUCTION_H

#include "ir/expression/constant/constant.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/variable_reference.h"
#include "ir/instruction/instruction_visitor.h"

#include <boost/iterator/indirect_iterator.hpp>

#include <iostream>
#include <memory>
#include <vector>

namespace ir {
    /*
     * Represents a compiled and labeled statement.
     */
    class Instruction {
    public:
        // An Enum of supported instructions. Makes it easier to distinguish instructions without having to
        // dynamic_cast them.
        enum class Kind { ASSIGNMENT = 1, HAVOC = 2, CALL = 3, GOTO = 5, IF = 7, SEQUENCE = 8, WHILE = 9 };

        virtual ~Instruction() = default;
        virtual void accept(InstructionVisitor &visitor) const = 0;
        virtual std::ostream &print(std::ostream &os) const = 0;
        friend std::ostream &operator<<(std::ostream &os, const Instruction &instruction) {
            return instruction.print(os);
        }
        std::unique_ptr<Instruction> clone() const {
            return std::unique_ptr<Instruction>(this->clone_implementation());
        }
        Kind getKind() const;

    protected:
        explicit Instruction(Kind kind);

    private:
        virtual Instruction *clone_implementation() const = 0;

    private:
        const Kind _kind;
    };
}// namespace ir

#endif//AHORN_INSTRUCTION_H
