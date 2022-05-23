#ifndef AHORN_STATEMENT_VISITOR_H
#define AHORN_STATEMENT_VISITOR_H

class AssertStatement;
class AssignmentStatement;
class AssumeStatement;
class CaseStatement;
class IfStatement;
class InvocationStatement;
class WhileStatement;

class StatementVisitor {
public:
    virtual void visit(AssertStatement &statement) = 0;
    virtual void visit(AssignmentStatement &statement) = 0;
    virtual void visit(AssumeStatement &statement) = 0;
    virtual void visit(CaseStatement &statement) = 0;
    virtual void visit(IfStatement &statement) = 0;
    virtual void visit(InvocationStatement &statement) = 0;
    virtual void visit(WhileStatement &statement) = 0;
};

#endif//AHORN_STATEMENT_VISITOR_H
