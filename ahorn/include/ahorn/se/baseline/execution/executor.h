#ifndef AHORN_BASELINE_EXECUTOR_H
#define AHORN_BASELINE_EXECUTOR_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "se/baseline/context/context.h"
#include "se/baseline/execution/encoder.h"
#include "se/baseline/execution/evaluator.h"
#include "se/baseline/z3/solver.h"

#include "boost/optional.hpp"

#include <map>
#include <memory>
#include <set>

namespace se::baseline {
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

        explicit Executor(Solver &solver);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        execute(std::unique_ptr<Context> context);

    private:
        void initialize(const Cfg &cfg);

        void handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        std::unique_ptr<Context> handleFunctionEntryVertex();
        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleProgramExitVertex(std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleFunctionBlockExitVertex(std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleFunctionExitVertex();

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        void tryFork(const State &state, const std::vector<z3::expr> &path_constraint, const z3::expr &expression,
                     const Vertex &vertex);

        bool containsUnconstrainedUninterpretedConstant(const State &state, const z3::expr &expression) const;

    private:
        Solver *const _solver;
        std::unique_ptr<Evaluator> _evaluator;
        std::unique_ptr<Encoder> _encoder;

        // "Globally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;
        std::set<std::string> _whole_program_inputs;

        std::unique_ptr<Context> _context;
        boost::optional<std::unique_ptr<Context>> _forked_context;
    };
}// namespace se::baseline

#endif//AHORN_BASELINE_EXECUTOR_H
