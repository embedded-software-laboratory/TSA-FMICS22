#include "ir/expression/variable_access.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"
#include "ir/type/derived_type.h"
#include "ir/type/elementary_type.h"
#include "ir/type/safety_type.h"

using namespace ir;

VariableAccess::VariableAccess(std::shared_ptr<Variable> variable)
    : VariableReference(Expression::Kind::VARIABLE_ACCESS), _variable(std::move(variable)) {
    const auto &data_type = _variable->getDataType();
    if (const auto *elementary_type = dynamic_cast<const ElementaryType *>(&data_type)) {
        switch (elementary_type->getType()) {
            case ElementaryType::Type::INT:
            case ElementaryType::Type::REAL: {
                _type = Expression::Type::ARITHMETIC;
                break;
            }
            case ElementaryType::Type::BOOL: {
                _type = Expression::Type::BOOLEAN;
                break;
            }
            case ElementaryType::Type::TIME: {
                _type = Expression::Type::ARITHMETIC;
                break;
            }
            default:
                throw std::logic_error("Invalid elementary type in VariableAccess.");
        }
    } else if (data_type.getKind() == DataType::Kind::ENUMERATED_TYPE) {
        _type = Expression::Type::ARITHMETIC;
    } else if (const auto *safety_type = dynamic_cast<const SafetyType *>(&data_type)) {
        switch (safety_type->getType()) {
            case SafetyType::Kind::SAFEBOOL: {
                _type = Expression::Type::BOOLEAN;
                break;
            }
            default:
                throw std::logic_error("Invalid safety type in VariableAccess.");
        }
    } else if (data_type.getKind() == DataType::Kind::DERIVED_TYPE) {
        _type = Expression::Type::UNDEFINED;
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

VariableAccess *VariableAccess::clone_implementation() const {
    return new VariableAccess(_variable);
}

void VariableAccess::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &VariableAccess::print(std::ostream &os) const {
    return os << getName();
}

std::shared_ptr<Variable> VariableAccess::getVariable() const {
    return _variable;
}

std::string VariableAccess::getName() const {
    return _variable->getName();
}

void VariableAccess::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}
