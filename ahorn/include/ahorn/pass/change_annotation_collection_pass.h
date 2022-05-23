#ifndef AHORN_CHANGE_ANNOTATION_COLLECTION_PASS_H
#define AHORN_CHANGE_ANNOTATION_COLLECTION_PASS_H

#include "cfg/cfg.h"
#include "ir/expression/expression_visitor.h"
#include "ir/instruction/instruction_visitor.h"

#include <set>

namespace pass {
    class ChangeAnnotationCollectionPass : private ir::InstructionVisitor, private ir::ExpressionVisitor {
    public:
        ChangeAnnotationCollectionPass() = default;

        std::set<unsigned int> apply(const Cfg &cfg);

    private:
        bool containsChangeExpression(const ir::Instruction &instruction);

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

        void visit(const ir::BinaryExpression &expression) override;
        void visit(const ir::BooleanToIntegerCast &expression) override;
        void visit(const ir::ChangeExpression &expression) override;
        void visit(const ir::UnaryExpression &expression) override;
        void visit(const ir::BooleanConstant &expression) override;
        void visit(const ir::IntegerConstant &expression) override;
        void visit(const ir::TimeConstant &expression) override;
        void visit(const ir::EnumeratedValue &expression) override;
        void visit(const ir::NondeterministicConstant &expression) override;
        void visit(const ir::Undefined &expression) override;
        void visit(const ir::VariableAccess &expression) override;
        void visit(const ir::FieldAccess &expression) override;
        void visit(const ir::IntegerToBooleanCast &expression) override;
        void visit(const ir::Phi &expression) override;

    private:
        bool _contains_change_expression;
    };
}// namespace pass

#endif//AHORN_CHANGE_ANNOTATION_COLLECTION_PASS_H
