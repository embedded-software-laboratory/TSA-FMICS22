#ifndef AHORN_OA_EXECUTOR_H
#define AHORN_OA_EXECUTOR_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "se/oa/execution/encoder.h"
#include "se/oa/z3/solver.h"

#include "boost/optional.hpp"

#include <map>

namespace se::oa {
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

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        execute(std::unique_ptr<Context> context);

    private:
        void initialize(const Cfg &cfg);

        void handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionEntryVertex();
        std::pair<std::unique_ptr<Context>, boost::optional<std::unique_ptr<Context>>>
        handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context);
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
        boost::optional<std::unique_ptr<Context>> tryFork(const State &state,
                                                          const std::vector<z3::expr> &path_constraint,
                                                          const z3::expr &expression, const Vertex &vertex);

    private:
        Solver *const _solver;
        std::unique_ptr<Encoder> _encoder;

        // "Globally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;

        std::unique_ptr<Context> _context;
        boost::optional<std::pair<std::unique_ptr<Context>, std::unique_ptr<Context>>> _forked_contexts;
    };
}// namespace se::oa

#endif//AHORN_OA_EXECUTOR_H
