#ifndef AHORN_CBMC_EXECUTOR_H
#define AHORN_CBMC_EXECUTOR_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "se/cbmc/context/context.h"
#include "se/cbmc/execution/encoder.h"
#include "se/cbmc/exploration/explorer.h"
#include "se/cbmc/z3/solver.h"

#include "boost/optional.hpp"

#include <memory>

namespace se::cbmc {
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

        Executor(Solver &solver, Explorer &explorer);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        execute(std::unique_ptr<Context> context);

    private:
        void initialize(const Cfg &cfg);

        void handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, unsigned int cycle, State &state,
                                      const Frame &frame);
        void handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, unsigned int cycle, State &state,
                                            const Frame &frame);
        void handleFunctionEntryVertex();
        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleProgramExitVertex(std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleFunctionBlockExitVertex(std::unique_ptr<Context> context);
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
        Solver *const _solver;
        Explorer *const _explorer;
        std::unique_ptr<Encoder> _encoder;

        std::set<std::string> _whole_program_inputs;

        std::unique_ptr<Context> _context;
        boost::optional<std::unique_ptr<Context>> _forked_context;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_EXECUTOR_H
