#include "ir/expression/field_access.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

using namespace ir;

FieldAccess::FieldAccess(std::unique_ptr<VariableAccess> record_access,
                         std::unique_ptr<VariableReference> variable_reference)
    : VariableReference(Expression::Kind::FIELD_ACCESS), _record_access(std::move(record_access)),
      _variable_reference(std::move(variable_reference)) {
    _type = _variable_reference->getType();
}

void FieldAccess::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &FieldAccess::print(std::ostream &os) const {
    return os << *_record_access << "." << *_variable_reference;
}

FieldAccess *FieldAccess::clone_implementation() const {
    auto record_access = _record_access->clone();
    auto variable_reference = _variable_reference->clone();
    return new FieldAccess(std::move(record_access), std::move(variable_reference));
}

const VariableReference &FieldAccess::getVariableReference() const {
    return *_variable_reference;
}

const VariableAccess &FieldAccess::getRecordAccess() const {
    return *_record_access;
}

std::string FieldAccess::getName() const {
    return _record_access->getName() + "." + _variable_reference->getName();
}

const VariableAccess &FieldAccess::getVariableAccess() const {
    if (_variable_reference->getKind() == Expression::Kind::VARIABLE_ACCESS) {
        return (VariableAccess &) *_variable_reference;
    } else if (const auto *field_access = dynamic_cast<const FieldAccess *>(_variable_reference.get())) {
        return field_access->getVariableAccess();
    } else {
        throw std::logic_error("Invalid expression in getFieldVariable().");
    }
}

void FieldAccess::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
