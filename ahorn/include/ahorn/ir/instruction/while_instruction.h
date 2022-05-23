#ifndef AHORN_WHILE_INSTRUCTION_H
#define AHORN_WHILE_INSTRUCTION_H

#include <gtest/gtest_prod.h>

#include "ir/expression/expression.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/instruction.h"
#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

class Builder;
class Compiler;

class TestLibIr_Module_Test;

namespace ir {
    /**
 * We distinguish between while and if instructions.
 * | while header | <--------------------|
 *        |         \                    |
 *        |           \                  |
 *        |             | body entry |   |
 *        |                    |         |
 *        |                    |         |
 *        |                    v         |
 *        |             | body exit | ---|
 * | while exit |
 */
    class WhileInstruction : public Instruction {
    private:
        friend Builder;
        friend Compiler;

    private:
        FRIEND_TEST(::TestLibIr, Module);

    public:
        WhileInstruction(std::unique_ptr<Expression> expression, std::unique_ptr<GotoInstruction> goto_body_entry,
                         std::unique_ptr<GotoInstruction> goto_while_exit);

        Expression &getExpression() const;

        void accept(InstructionVisitor &visitor) const override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<WhileInstruction> clone() const {
            return std::unique_ptr<WhileInstruction>(static_cast<WhileInstruction *>(this->clone_implementation()));
        }

    private:
        GotoInstruction &getGotoWhileExit() const;

        int getBodyEntryLabel() const;
        int getWhileExitLabel() const;

    private:
        WhileInstruction *clone_implementation() const override;

    private:
        const std::unique_ptr<Expression> _expression;
        std::unique_ptr<GotoInstruction> _goto_body_entry;
        std::unique_ptr<GotoInstruction> _goto_while_exit;
    };
}// namespace ir

#endif//AHORN_WHILE_INSTRUCTION_H
