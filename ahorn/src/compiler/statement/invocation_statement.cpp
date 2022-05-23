#include "compiler/statement/invocation_statement.h"

#include <algorithm>
#include <sstream>

using namespace ir;

InvocationStatement::InvocationStatement(std::unique_ptr<VariableAccess> variable_access,
                                         std::vector<std::unique_ptr<AssignmentStatement>> pre_assignments,
                                         std::vector<std::unique_ptr<AssignmentStatement>> post_assignments)
    : _variable_access(std::move(variable_access)), _pre_assignments(std::move(pre_assignments)),
      _post_assignments(std::move(post_assignments)) {}

std::ostream &InvocationStatement::print(std::ostream &os) const {
    return os << "InvocationStatement" << std::endl;
}

void InvocationStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

const VariableAccess &InvocationStatement::getVariableAccess() const {
    return *_variable_access;
}

boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
InvocationStatement::preAssignmentsBegin() const {
    return std::begin(_pre_assignments);
}

boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
InvocationStatement::preAssignmentsEnd() const {
    return std::end(_pre_assignments);
}

boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
InvocationStatement::postAssignmentsBegin() const {
    return std::begin(_post_assignments);
}

boost::indirect_iterator<std::vector<std::unique_ptr<AssignmentStatement>>::const_iterator>
InvocationStatement::postAssignmentsEnd() const {
    return std::end(_post_assignments);
}

InvocationStatement *InvocationStatement::clone_implementation() const {
    std::unique_ptr<VariableAccess> variable_access = _variable_access->clone();
    std::vector<std::unique_ptr<AssignmentStatement>> pre_assignments;
    for (const auto &statement : _pre_assignments) {
        pre_assignments.emplace_back(std::move(statement->clone()));
    }
    std::vector<std::unique_ptr<AssignmentStatement>> post_assignments;
    for (const auto &statement : _post_assignments) {
        post_assignments.emplace_back(std::move(statement->clone()));
    }
    return new InvocationStatement(std::move(variable_access), std::move(pre_assignments), std::move(post_assignments));
}

bool InvocationStatement::hasPostAssignmentStatements() const {
    return !_post_assignments.empty();
}
