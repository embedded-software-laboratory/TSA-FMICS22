#ifndef AHORN_INSTRUCTION_VISITOR_H
#define AHORN_INSTRUCTION_VISITOR_H

namespace ir {
    class AssignmentInstruction;
    class CallInstruction;
    class IfInstruction;
    class SequenceInstruction;
    class WhileInstruction;
    class GotoInstruction;
    class HavocInstruction;

    class InstructionVisitor {
    public:
        virtual ~InstructionVisitor() = default;

        virtual void visit(const AssignmentInstruction &instruction) = 0;
        virtual void visit(const CallInstruction &instruction) = 0;
        virtual void visit(const IfInstruction &instruction) = 0;
        virtual void visit(const SequenceInstruction &instruction) = 0;
        virtual void visit(const WhileInstruction &instruction) = 0;
        virtual void visit(const GotoInstruction &instruction) = 0;
        virtual void visit(const HavocInstruction &instruction) = 0;
    };
}// namespace ir

#endif//AHORN_INSTRUCTION_VISITOR_H
