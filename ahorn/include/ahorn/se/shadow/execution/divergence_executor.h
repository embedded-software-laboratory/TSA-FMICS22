#ifndef AHORN_SHADOW_DIVERGENCE_EXECUTOR_H
#define AHORN_SHADOW_DIVERGENCE_EXECUTOR_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "se/shadow/execution/divergence_encoder.h"
#include "se/shadow/execution/divergence_evaluator.h"
#include "se/shadow/z3/solver.h"

#include "boost/optional.hpp"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace se::shadow {
    class DivergenceExecutor : private ir::InstructionVisitor {
    private:
        friend class Engine;

    public:
        enum class ExecutionStatus { EXPECTED_BEHAVIOR, DIVERGENT_BEHAVIOR, POTENTIAL_DIVERGENT_BEHAVIOR };

    public:
        // XXX default constructor disabled
        DivergenceExecutor() = delete;
        // XXX copy constructor disabled
        DivergenceExecutor(const DivergenceExecutor &other) = delete;
        // XXX copy assignment disabled
        DivergenceExecutor &operator=(const DivergenceExecutor &) = delete;

        explicit DivergenceExecutor(Solver &solver);

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        std::pair<ExecutionStatus, std::vector<std::unique_ptr<Context>>> execute(Context &context);

    private:
        void initialize(const Cfg &cfg);

        void reset();

        void handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state);
        void handleFunctionEntryVertex();
        ExecutionStatus handleRegularVertex(const Vertex &vertex, Context &context);
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
        void extractNecessaryHardConstraints(std::set<std::string> &necessary_hard_constraints,
                                             const DivergentState &divergent_state, const z3::expr &expression) const;

        void tryDivergentFork(const DivergentState &divergent_state, const z3::expr &old_encoded_expression,
                              const z3::expr &new_encoded_expression, const Vertex &vertex);

        bool containsShadowExpression(const DivergentState &divergent_state, const z3::expr &expression) const;

    private:
        Solver *const _solver;
        std::unique_ptr<DivergenceEncoder> _encoder;
        std::unique_ptr<DivergenceEvaluator> _evaluator;
        // "Globally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;

        std::set<std::string> _whole_program_inputs;

        ExecutionStatus _execution_status;
        Context *_context;
        boost::optional<std::vector<std::unique_ptr<Context>>> _divergent_contexts;
    };
}// namespace se::shadow

#endif//AHORN_SHADOW_DIVERGENCE_EXECUTOR_H
