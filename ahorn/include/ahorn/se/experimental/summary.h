#ifndef AHORN_SUMMARY_H
#define AHORN_SUMMARY_H

#include "se/experimental/context/state.h"
#include "se/experimental/expression/assumption_literal_expression.h"
#include "se/experimental/expression/symbolic_expression.h"

#include <map>
#include <memory>
#include <vector>

namespace se {
    class Summary {
    public:
        // XXX default constructor disabled
        Summary() = delete;
        // XXX copy constructor disabled
        Summary(const Summary &other) = delete;
        // XXX copy assignment disabled
        Summary &operator=(const Summary &) = delete;

        Summary(std::shared_ptr<AssumptionLiteralExpression> entry_assumption_literal,
                std::shared_ptr<AssumptionLiteralExpression> exit_assumption_literal,
                std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator>
                        assumption_literals,
                std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                        assumptions,
                std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>
                        hard_constraints);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Summary &summary) {
            return summary.print(os);
        }

        const AssumptionLiteralExpression &getEntryAssumptionLiteral() const;

        const AssumptionLiteralExpression &getExitAssumptionLiteral() const;

        const std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator> &
        getAssumptionLiterals() const;

        const std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator> &
        getAssumptions() const;

        const std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> &
        getHardConstraints() const;

    private:
        std::shared_ptr<AssumptionLiteralExpression> _entry_assumption_literal;
        std::shared_ptr<AssumptionLiteralExpression> _exit_assumption_literal;
        std::map<std::string, std::shared_ptr<AssumptionLiteralExpression>, VerificationConditionComparator>
                _assumption_literals;
        std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                _assumptions;
        std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> _hard_constraints;
    };
}// namespace se

#endif//AHORN_SUMMARY_H
