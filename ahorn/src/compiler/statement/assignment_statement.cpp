#include "compiler/statement/assignment_statement.h"

#include <sstream>

using namespace ir;

AssignmentStatement::AssignmentStatement(std::unique_ptr<VariableReference> variable_reference,
                                         std::unique_ptr<Expression> expression)
    : _variable_reference(std::move(variable_reference)), _expression(std::move(expression)) {}

std::ostream &AssignmentStatement::print(std::ostream &os) const {
    std::stringstream str;
    str << *_variable_reference << " := " << *_expression;
    return os << str.str();
}

void AssignmentStatement::accept(StatementVisitor &visitor) {
    visitor.visit(*this);
}

const VariableReference &AssignmentStatement::getVariableReference() const {
    return *_variable_reference;
}

Expression &AssignmentStatement::getExpression() const {
    return *_expression;
}

AssignmentStatement *AssignmentStatement::clone_implementation() const {
    std::unique_ptr<VariableReference> variable_reference = _variable_reference->clone();
    std::unique_ptr<Expression> expression = _expression->clone();
    return new AssignmentStatement(std::move(variable_reference), std::move(expression));
}

void AssignmentStatement::setExpression(std::unique_ptr<Expression> expression) {
    _expression = std::move(expression);
}
