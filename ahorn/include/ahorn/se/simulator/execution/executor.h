#ifndef AHORN_SIMULATOR_EXECUTOR_H
#define AHORN_SIMULATOR_EXECUTOR_H

#include "ir/instruction/instruction_visitor.h"
#include "se/shadow/z3/solver.h"
#include "se/simulator/context/context.h"
#include "se/simulator/execution/evaluator.h"

namespace se::simulator {
    class Executor : private ir::InstructionVisitor {
    private:
        friend class Engine;

    public:
        // XXX default constructor disabled
        Executor() = delete;
        // XXX copy constructor disabled
        Executor(const Executor &other) = delete;
        // XXX copy assignment disabled
        Executor &operator=(const Executor &) = delete;

        Executor(Evaluator::ShadowProcessingMode shadow_processing_mode, shadow::Solver &solver);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        void execute(Context &context);

    private:
        void initialize(const Cfg &cfg);

        void handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionEntryVertex();
        void handleRegularVertex(const Vertex &vertex, Context &context);
        void handleProgramExitVertex(const Frame &frame, Context &context, State &state);
        void handleFunctionBlockExitVertex(const Frame &frame, Context &context, State &state);
        void handleFunctionExitVertex();

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        shadow::Solver *const _solver;
        std::unique_ptr<Evaluator> _evaluator;

        std::set<std::string> _whole_program_inputs;

        Context *_context;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_EXECUTOR_H
