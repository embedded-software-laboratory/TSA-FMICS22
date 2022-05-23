#ifndef AHORN_COMPILER_H
#define AHORN_COMPILER_H

#include <gtest/gtest_prod.h>

#include "compiler/pou/pou.h"
#include "compiler/project.h"
#include "ir/module.h"
#include "ir/project.h"

#include <map>
#include <memory>
#include <stack>

class TestLibCompiler_Compiler_Test;

/// The compiler consists of four stages:
/// Stage 1 and 2 - Lexical and syntactical analysis (via ANTLR4)
/// TODO: Stage 3 (Semantic analysis)
/// Stage 4 - Intermediate code generation
class Compiler {
    friend class Ast;

public:
    Compiler() = default;

    // Stage 1 and 2 - Lexical and syntactical analysis (via ANTLR4)
    static std::unique_ptr<compiler::Project> parse(const std::string &source_code);

    // Stage 3 - Semantic analysis
    static std::unique_ptr<compiler::Project> analyze(std::unique_ptr<compiler::Project> project);

    // Stage 4 - Intermediate code generation
    static std::unique_ptr<Project> compile(const std::string &source_code);

private:
    FRIEND_TEST(::TestLibCompiler, Compiler);

    class PouCompiler : private StatementVisitor {
    private:
        using variable_vector_t = std::vector<std::shared_ptr<ir::Variable>>;
        using variable_map_t = std::map<std::string, std::shared_ptr<ir::Variable>>;
        using module_ref_t = std::reference_wrapper<const ir::Module>;
        using module_ref_map_t = std::map<std::string, module_ref_t>;
        using instruction_ref_t = std::reference_wrapper<const ir::Instruction>;
        using instruction_ref_vector_t = std::vector<instruction_ref_t>;
        using instruction_ref_map_t = std::map<std::string, instruction_ref_t>;

    public:
        PouCompiler(const compiler::Project &project)
            : _project(project), _entry_offset(0), _label(0), _next_label_placeholder(-1), _last_statement(false){};

        std::unique_ptr<Project> compile();

    private:
        void compile(const Pou &pou);

        void visit(AssertStatement &statement) override;
        void visit(AssignmentStatement &statement) override;
        void visit(AssumeStatement &statement) override;
        void visit(CaseStatement &statement) override;
        void visit(IfStatement &statement) override;
        void visit(InvocationStatement &statement) override;
        void visit(WhileStatement &statement) override;

    private:
        const compiler::Project &_project;
        std::map<std::string, std::unique_ptr<ir::Module>> _name_to_module;
        std::unique_ptr<ir::Interface> _interface;
        std::map<int, std::unique_ptr<ir::Instruction>> _label_to_instruction;

        int _entry_offset;
        int _label;
        int _next_label_placeholder;
        std::stack<int> _next_label_placeholders;
        bool _last_statement;
    };
};

#endif//AHORN_COMPILER_H
