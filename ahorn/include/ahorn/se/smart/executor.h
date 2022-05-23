#ifndef AHORN_SMART_EXECUTOR_H
#define AHORN_SMART_EXECUTOR_H

#include "ir/instruction/instruction_visitor.h"
#include "se/smart/context/context.h"
#include "se/smart/encoder.h"
#include "se/smart/evaluator.h"
#include "se/smart/z3/manager.h"

#include <memory>
#include <stack>

namespace se::smart {
    class Executor : private ir::InstructionVisitor {
    public:
        // XXX default constructor disabled
        Executor() = delete;
        // XXX copy constructor disabled
        Executor(const Executor &other) = delete;
        // XXX copy assignment disabled
        Executor &operator=(const Executor &) = delete;

        explicit Executor(Manager &manager);

        std::unique_ptr<Context> execute(std::unique_ptr<Context> context, const std::stack<unsigned int> &stack);

    private:
        std::unique_ptr<Context> handleProgramEntry(const Cfg &cfg, const Vertex &vertex,
                                                    std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleFunctionBlockEntry(const Cfg &cfg, const Vertex &vertex,
                                                          std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleRegularVertex(const Vertex &vertex, std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleFunctionBlockExit(const Cfg &cfg, const Vertex &vertex,
                                                         std::unique_ptr<Context> context);
        std::unique_ptr<Context> handleProgramExit(const Cfg &cfg, const Vertex &vertex,
                                                   std::unique_ptr<Context> context);

        std::unique_ptr<Context> solvePathConstraint(std::unique_ptr<Context> context);

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        Manager *const _manager;
        bool _backtrack;
        std::unique_ptr<Evaluator> _evaluator;
        std::unique_ptr<Encoder> _encoder;
        std::unique_ptr<Context> _context;
    };
}// namespace se::smart

#endif//AHORN_SMART_EXECUTOR_H
