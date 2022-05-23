#ifndef AHORN_EXECUTOR_H
#define AHORN_EXECUTOR_H

#include <gtest/gtest_prod.h>

#include "cfg/vertex.h"
#include "ir/instruction/instruction.h"
#include "ir/instruction/instruction_visitor.h"
#include "se/experimental/configuration.h"
#include "se/experimental/context/context.h"
#include "se/experimental/encoder.h"
#include "se/experimental/evaluator.h"
#include "se/experimental/summarizer.h"
#include "se/experimental/z3/manager.h"

#include "boost/optional/optional_io.hpp"

#include <memory>

class TestLibSe_Executor_Test;

namespace se {
    class Executor : private ir::InstructionVisitor {
    private:
        FRIEND_TEST(::TestLibSe, Executor);

    public:
        enum class ExecutionStatus { EXPECTED_BEHAVIOR, DIVERGENT_BEHAVIOR, POTENTIAL_DIVERGENT_BEHAVIOR };

        // XXX default constructor disabled
        Executor() = delete;
        // XXX copy constructor disabled
        Executor(const Executor &other) = delete;
        // XXX copy assignment disabled
        Executor &operator=(const Executor &) = delete;

        Executor(Configuration &configuration, Manager &manager);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Executor &executor) {
            return executor.print(os);
        }

        std::pair<ExecutionStatus, std::vector<std::unique_ptr<Context>>> execute(std::unique_ptr<Context> context);

    private:
        std::unique_ptr<Context> handleProgramEntry(const Cfg &cfg, const Vertex &vertex,
                                                    std::unique_ptr<Context> context);

        std::unique_ptr<Context> handleFunctionBlockEntry(const Cfg &cfg, const Vertex &vertex,
                                                          std::unique_ptr<Context> context);

        std::pair<ExecutionStatus, std::vector<std::unique_ptr<Context>>>
        handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context);

        // prepares execution context for the next cycle
        std::unique_ptr<Context> handleProgramExit(const Cfg &cfg, const Vertex &vertex,
                                                   std::unique_ptr<Context> context);

        std::unique_ptr<Context> handleFunctionBlockExit(const Cfg &cfg, const Vertex &vertex,
                                                         std::unique_ptr<Context> context);

        void tryFork(std::shared_ptr<Expression> expression, const Vertex &next_vertex);

        void tryDivergentFork(std::shared_ptr<Expression> old_expression, std::shared_ptr<Expression> new_expression);

        bool containsShadowExpression(const Expression &expression) const;

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        inline static const std::string ASSUMPTION_LITERAL_PREFIX = "b_";

        Configuration *const _configuration;

        ExecutionStatus _execution_status;

        Manager *const _manager;

        std::unique_ptr<Evaluator> _evaluator;

        std::unique_ptr<Encoder> _encoder;

        std::unique_ptr<Summarizer> _summarizer;

        std::unique_ptr<Context> _context;

        boost::optional<std::unique_ptr<Context>> _forked_context;

        boost::optional<std::vector<std::unique_ptr<Context>>> _divergent_contexts;

        // memoization
        std::map<unsigned int, std::vector<z3::expr>> _id_to_uninterpreted_constants;
        std::map<unsigned int, std::shared_ptr<Expression>> _id_to_lowered_expression;
    };
}// namespace se

#endif//AHORN_EXECUTOR_H
