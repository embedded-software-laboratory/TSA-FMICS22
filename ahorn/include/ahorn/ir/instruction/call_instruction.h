#ifndef AHORN_CALL_INSTRUCTION_H
#define AHORN_CALL_INSTRUCTION_H

#include <gtest/gtest_prod.h>

#include "ir/expression/variable_access.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/instruction.h"
#include "ir/module.h"
#include "ir/variable.h"

#include <boost/iterator/indirect_iterator.hpp>
#include <memory>

class Builder;
class Compiler;

class TestLibCfg_Call_Test;

namespace ir {
    // a call instruction represents the call site of a function or function block, i.e, it is the location within the
    // caller where the function or function block (the callee) is called from.
    // the goto_intraprocedural represents the label of the return site
    class CallInstruction : public Instruction {
    private:
        friend Builder;
        friend Compiler;

    private:
        FRIEND_TEST(::TestLibCfg, Call);

    public:
        // The variable access is the call variable within the caller.
        // The goto_intraprocedural jumps to the label of the succeeding instruction within the caller after to which the
        // callee returns. Do note that for some data-flow analysis, a virtual intra-procedural edge from the call
        // instruction to the next instruction within the callsite is required, but not in control-flow analysis.
        // The goto_interprocedural_entry jumps to the entry of the callee.
        CallInstruction(std::unique_ptr<VariableAccess> variable_access,
                        std::unique_ptr<GotoInstruction> goto_intraprocedural,
                        std::unique_ptr<GotoInstruction> goto_interprocedural_entry);

        const VariableAccess &getVariableAccess() const;

        void accept(InstructionVisitor &visitor) const override;
        std::ostream &print(std::ostream &os) const override;

        void addLabelOfPreAssignment(int label);

        void addLabelOfPostAssignment(int label);

        std::unique_ptr<CallInstruction> clone() const {
            return std::unique_ptr<CallInstruction>(static_cast<CallInstruction *>(this->clone_implementation()));
        }

    private:
        GotoInstruction &getGotoIntraprocedural() const;
        int getIntraproceduralLabel() const;
        GotoInstruction &getGotoInterprocedural() const;
        int getInterproceduralLabel() const;

        std::vector<int> getLabelsOfPreAssignments() const;
        std::vector<int> getLabelsOfPostAssignments() const;

    private:
        CallInstruction *clone_implementation() const override;

    private:
        std::unique_ptr<VariableAccess> _variable_access;
        std::unique_ptr<GotoInstruction> _goto_intraprocedural;
        std::unique_ptr<GotoInstruction> _goto_interprocedural;
        std::vector<int> _labels_of_pre_assignments;
        std::vector<int> _labels_of_post_assignments;
    };
}// namespace ir

#endif//AHORN_CALL_INSTRUCTION_H
