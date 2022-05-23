#ifndef AHORN_IF_INSTRUCTION_H
#define AHORN_IF_INSTRUCTION_H

#include <gtest/gtest_prod.h>

#include "ir/expression/expression.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/instruction.h"
#include <boost/iterator/indirect_iterator.hpp>

#include "boost/optional/optional.hpp"

#include <memory>
#include <vector>

class Builder;
class Compiler;

namespace pass {
    class TacPass;
    class SsaPass;
}

class TestLibIr_Module_Test;
class TestLibCfg_If_Test;

namespace ir {
    class IfInstruction : public Instruction {
    private:
        friend Builder;
        friend Compiler;
        friend class pass::TacPass;
        friend class pass::SsaPass;

    private:
        FRIEND_TEST(::TestLibIr, Module);
        FRIEND_TEST(::TestLibCfg, If);

    public:
        IfInstruction(std::unique_ptr<Expression> expression, std::unique_ptr<GotoInstruction> goto_then,
                      std::unique_ptr<GotoInstruction> goto_else);

        Expression &getExpression() const;

        void accept(InstructionVisitor &visitor) const override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<IfInstruction> clone() const {
            return std::unique_ptr<IfInstruction>(static_cast<IfInstruction *>(this->clone_implementation()));
        }

    private:
        // XXX control-flow is hidden, access should always be via corresponding cfg
        GotoInstruction &getGotoThen() const;
        GotoInstruction &getGotoElse() const;

        int getThenLabel() const;
        int getElseLabel() const;

    private:
        IfInstruction *clone_implementation() const override;

    private:
        std::unique_ptr<Expression> _expression;
        std::unique_ptr<GotoInstruction> _goto_then;
        std::unique_ptr<GotoInstruction> _goto_else;
    };
}// namespace ir

#endif//AHORN_IF_INSTRUCTION_H
