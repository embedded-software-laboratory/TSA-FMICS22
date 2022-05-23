#include "ir/expression/phi.h"
#include "ir/expression/expression_visitor.h"
#include "ir/expression/non_const_expression_visitor.h"

#include <sstream>

using namespace ir;

Phi::Phi(std::shared_ptr<Variable> variable, std::map<unsigned int, std::unique_ptr<VariableAccess>> label_to_operand)
    : Expression(Kind::PHI), _variable(std::move(variable)), _label_to_operand(std::move(label_to_operand)) {}

void Phi::accept(ExpressionVisitor &visitor) const {
    visitor.visit(*this);
}

std::ostream &Phi::print(std::ostream &os) const {
    std::stringstream str;
    str << "PHI(";
    for (auto it = _label_to_operand.begin(); it != _label_to_operand.end(); ++it) {
        str << "L" << std::to_string(it->first) << ": " << *it->second;
        if (std::next(it) != _label_to_operand.end()) {
            str << ", ";
        }
    }
    str << ")";
    return os << str.str();
}

Phi *Phi::clone_implementation() const {
    throw std::logic_error("Not implemented yet.");
}

std::shared_ptr<Variable> Phi::getVariable() const {
    return _variable;
}

void Phi::accept(NonConstExpressionVisitor &visitor) {
    visitor.visit(*this);
}

const std::map<unsigned int, std::unique_ptr<VariableAccess>> &Phi::getLabelToOperand() const {
    return _label_to_operand;
}
