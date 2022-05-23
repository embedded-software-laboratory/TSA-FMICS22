#ifndef AHORN_INVOCATION_STATEMENT_H
#define AHORN_INVOCATION_STATEMENT_H

#include <gtest/gtest_prod.h>

#include "assignment_statement.h"
#include "compiler/pou/pou.h"
#include "ir/expression/variable_access.h"
#include "statement.h"
#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

class TestLibCompiler_Compiler_Test;

class InvocationStatement : public Statement {
private:
    FRIEND_TEST(::TestLibCompiler, Compiler);

public:
    explicit InvocationStatement(std::unique_ptr<ir::VariableAccess> variable_access,
                                 std::vector<std::unique_ptr<AssignmentStatement>> pre_assignments,
                                 std::vector<std::unique_ptr<AssignmentStatement>> post_assignments);
    const ir::VariableAccess &getVariableAccess() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
    preAssignmentsBegin() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
    preAssignmentsEnd() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
    postAssignmentsBegin() const;
    boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
    postAssignmentsEnd() const;
    bool hasPostAssignmentStatements() const;
    void accept(StatementVisitor &visitor) override;
    std::ostream &print(std::ostream &os) const override;
    std::unique_ptr<InvocationStatement> clone() const {
        return std::unique_ptr<InvocationStatement>(static_cast<InvocationStatement *>(this->clone_implementation()));
    }

private:
    InvocationStatement *clone_implementation() const override;

private:
    std::unique_ptr<ir::VariableAccess> const _variable_access;
    const std::vector<std::unique_ptr<AssignmentStatement>> _pre_assignments;
    const std::vector<std::unique_ptr<AssignmentStatement>> _post_assignments;
};

#endif//AHORN_INVOCATION_STATEMENT_H
