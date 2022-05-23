#ifndef AHORN_ANALYZER_H
#define AHORN_ANALYZER_H

#include "compiler/project.h"
#include "compiler/statement/statement_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

class Analyzer : private StatementVisitor, private ir::NonConstExpressionVisitor {
public:
    explicit Analyzer(compiler::Project &project) : _project(project){};

    void analyze();

private:
    void analyze(Pou &pou);

private:
    compiler::Project &_project;
    // currently analyzed pou
    Pou *_pou;

private:
    void visit(AssertStatement &statement) override;
    void visit(AssignmentStatement &statement) override;
    void visit(AssumeStatement &statement) override;
    void visit(CaseStatement &statement) override;
    void visit(IfStatement &statement) override;
    void visit(InvocationStatement &statement) override;
    void visit(WhileStatement &statement) override;

    void visit(ir::BinaryExpression &expression) override;
    void visit(ir::BooleanToIntegerCast &expression) override;
    void visit(ir::ChangeExpression &expression) override;
    void visit(ir::UnaryExpression &expression) override;
    void visit(ir::BooleanConstant &expression) override;
    void visit(ir::IntegerConstant &expression) override;
    void visit(ir::TimeConstant &expression) override;
    void visit(ir::EnumeratedValue &expression) override;
    void visit(ir::NondeterministicConstant &expression) override;
    void visit(ir::Undefined &expression) override;
    void visit(ir::VariableAccess &expression) override;
    void visit(ir::FieldAccess &expression) override;
    void visit(ir::IntegerToBooleanCast &expression) override;
    void visit(ir::Phi &expression) override;
};

#endif//AHORN_ANALYZER_H
