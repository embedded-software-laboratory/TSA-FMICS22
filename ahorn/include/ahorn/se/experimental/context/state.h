#ifndef AHORN_STATE_H
#define AHORN_STATE_H

#include <gtest/gtest_prod.h>

#include "cfg/vertex.h"
#include "se/experimental/configuration.h"
#include "se/experimental/expression/assumption_literal_expression.h"
#include "se/experimental/expression/concrete_expression.h"
#include "se/experimental/expression/expression.h"
#include "se/experimental/expression/shadow_expression.h"
#include "se/experimental/expression/symbolic_expression.h"

#include "boost/optional.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <set>

class TestLibSe_State_Test;

namespace se {
    class Manager;

    class ContextualizedNameComparator {
    public:
        bool operator()(const std::string &contextualized_name_1, const std::string &contextualized_name_2) const {
            std::size_t version_position_1 = contextualized_name_1.find('_');
            std::size_t version_position_2 = contextualized_name_2.find('_');
            std::size_t cycle_position_1 = contextualized_name_1.find("__");
            std::size_t cycle_position_2 = contextualized_name_2.find("__");
            std::string flattened_name_1 = contextualized_name_1.substr(0, version_position_1);
            std::string flattened_name_2 = contextualized_name_2.substr(0, version_position_2);
            std::size_t scope_count_1 = std::count(contextualized_name_1.begin(), contextualized_name_1.end(), '.');
            std::size_t scope_count_2 = std::count(contextualized_name_2.begin(), contextualized_name_2.end(), '.');
            unsigned int version_1 = std::stoi(
                    contextualized_name_1.substr(version_position_1 + 1, cycle_position_1 - version_position_1 - 1));
            unsigned int version_2 = std::stoi(
                    contextualized_name_2.substr(version_position_2 + 1, cycle_position_2 - version_position_2 - 1));
            unsigned int cycle_1 =
                    std::stoi(contextualized_name_1.substr(cycle_position_1 + 2, contextualized_name_1.size()));
            unsigned int cycle_2 =
                    std::stoi(contextualized_name_2.substr(cycle_position_2 + 2, contextualized_name_2.size()));
            if (cycle_1 < cycle_2) {
                return true;
            } else if (cycle_1 == cycle_2) {
                if (scope_count_1 < scope_count_2) {
                    return true;
                } else if (scope_count_1 == scope_count_2) {
                    if (flattened_name_1 < flattened_name_2) {
                        return true;
                    } else if (flattened_name_1 == flattened_name_2) {
                        if (version_1 < version_2) {
                            return true;
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    };

    class VerificationConditionComparator {
    public:
        bool operator()(const std::string &assumption_literal_name_1,
                        const std::string &assumption_literal_name_2) const {
            std::size_t scope_position_1 = assumption_literal_name_1.find('_') + 1;
            std::size_t scope_position_2 = assumption_literal_name_2.find('_') + 1;
            std::size_t version_position_1 = assumption_literal_name_1.find('_', scope_position_1) + 1;
            std::size_t version_position_2 = assumption_literal_name_2.find('_', scope_position_2) + 1;
            std::string scope_1 =
                    assumption_literal_name_1.substr(scope_position_1, version_position_1 - scope_position_1 - 1);
            std::string scope_2 =
                    assumption_literal_name_2.substr(scope_position_2, version_position_2 - scope_position_2 - 1);
            std::size_t cycle_position_1 = assumption_literal_name_1.find("__");
            std::size_t cycle_position_2 = assumption_literal_name_2.find("__");
            if (cycle_position_1 == std::string::npos && cycle_position_2 == std::string::npos) {
                unsigned int version_1 = std::stoi(
                        assumption_literal_name_1.substr(version_position_1, assumption_literal_name_1.size()));
                unsigned int version_2 = std::stoi(
                        assumption_literal_name_2.substr(version_position_2, assumption_literal_name_2.size()));
                if (version_1 < version_2) {
                    return true;
                } else {
                    return false;
                }
            } else {
                unsigned int version_1 = std::stoi(
                        assumption_literal_name_1.substr(version_position_1, cycle_position_1 - version_position_1));
                unsigned int version_2 = std::stoi(
                        assumption_literal_name_2.substr(version_position_2, cycle_position_2 - version_position_2));
                unsigned int cycle_1 = std::stoi(
                        assumption_literal_name_1.substr(cycle_position_1 + 2, assumption_literal_name_1.size()));
                unsigned int cycle_2 = std::stoi(
                        assumption_literal_name_2.substr(cycle_position_2 + 2, assumption_literal_name_2.size()));
                if (cycle_1 < cycle_2) {
                    return true;
                } else if (cycle_1 == cycle_2) {
                    if (version_1 < version_2) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
    };

    class State {
    private:
        FRIEND_TEST(::TestLibSe, State);

    public:
        // XXX default constructor disabled
        State() = delete;
        // XXX copy constructor disabled
        State(const State &other) = delete;
        // XXX copy assignment disabled
        State &operator=(const State &) = delete;

        State(const Configuration &configuration, const Vertex &vertex,
              std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                      contextualized_name_to_concrete_expression,
              std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                      contextualized_name_to_symbolic_expression,
              std::map<std::string, std::shared_ptr<ShadowExpression>> shadow_name_to_shadow_expression,
              std::vector<std::shared_ptr<Expression>> path_constraint,
              std::shared_ptr<AssumptionLiteralExpression> assumption_literal,
              std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                       VerificationConditionComparator>
                      assumption_literals,
              std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                      assumptions,
              std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> hard_constraints,
              std::map<std::string, std::string, VerificationConditionComparator>
                      unknown_over_approximating_summary_literals);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const State &state) {
            return state.print(os);
        }

        const Vertex &getVertex() const;

        void setVertex(const Vertex &vertex);

        std::map<std::string, std::shared_ptr<Expression>>::const_iterator concreteStoreBegin() const;

        std::map<std::string, std::shared_ptr<Expression>>::const_iterator concreteStoreEnd() const;

        const std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator> &
        getConcreteStore() const;

        std::shared_ptr<Expression> getConcreteExpression(const std::string &contextualized_name) const;

        void setConcreteExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression);

        void updateConcreteExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression);

        std::map<std::string, std::shared_ptr<Expression>>::const_iterator symbolicStoreBegin() const;

        std::map<std::string, std::shared_ptr<Expression>>::const_iterator symbolicStoreEnd() const;

        const std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator> &
        getSymbolicStore() const;

        std::shared_ptr<Expression> getSymbolicExpression(const std::string &contextualized_name) const;

        void setSymbolicExpression(const std::string &contextualized_name, std::shared_ptr<Expression> expression);

        const std::map<std::string, std::shared_ptr<ShadowExpression>> &getShadowStore() const;

        std::shared_ptr<ShadowExpression> getShadowExpression(const std::string &shadow_name) const;

        void setShadowExpression(const std::string &shadow_name, std::shared_ptr<ShadowExpression> shadow_expression);

        // TODO (16.12.21): Refactor to begin()- and end() iterators.
        const std::vector<std::shared_ptr<Expression>> &getPathConstraint() const;

        void pushPathConstraint(std::shared_ptr<Expression> expression);

        void clearPathConstraint();

        std::shared_ptr<AssumptionLiteralExpression> getAssumptionLiteral() const;

        void setAssumptionLiteral(std::shared_ptr<AssumptionLiteralExpression> assumption_literal);

        const std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                       VerificationConditionComparator> &
        getAssumptionLiterals() const;

        const std::vector<std::shared_ptr<AssumptionLiteralExpression>> &
        getAssumptionLiterals(const std::string &assumption_literal_name) const;

        void pushAssumptionLiteral(std::string assumption_literal_name,
                                   std::shared_ptr<AssumptionLiteralExpression> assumption_literal);

        // TODO (18.01.21): Expose iterators?
        const std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator> &
        getAssumptions() const;

        const std::vector<std::shared_ptr<SymbolicExpression>> &
        getAssumptions(const std::string &assumption_literal_name) const;

        void pushAssumption(std::string assumption_literal_name, std::shared_ptr<Expression> assumption);

        const std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> &
        getHardConstraints() const;

        const std::map<std::string, z3::expr> &getHardConstraints(const std::string &assumption_literal_name) const;

        void pushHardConstraint(std::string assumption_literal_name, const std::string &contextualized_name,
                                const z3::expr &z3_expression);

        const std::map<std::string, std::string, VerificationConditionComparator> &
        getUnknownOverApproximatingSummaryLiterals() const;

        void pushUnknownOverApproximatingSummaryLiteral(const std::string &caller_assumption_literal_name,
                                                        const std::string &callee_assumption_literal_name);

        std::unique_ptr<State> fork(Manager &manager, z3::model &z3_model);

        unsigned int extractVersion(const std::string &contextualized_name) const;

        unsigned int extractCycle(const std::string &contextualized_name) const;

        void removeIntermediateVersions(const Manager &manager, unsigned int cycle);

        bool isShadowConstant(const std::string &name) const;

        unsigned int getMaximumVersionFromConcreteStore(const std::string &flattened_name, unsigned int cycle) const;

        unsigned int getMinimumVersionFromConcreteStore(const std::string &flattened_name, unsigned int cycle) const;

        unsigned int getMaximumVersionFromSymbolicStore(const std::string &flattened_name, unsigned int cycle) const;

        unsigned int getMinimumVersionFromSymbolicStore(const std::string &flattened_name, unsigned int cycle) const;

    private:
        unsigned int getMinimumVersion(bool from_concrete_store, const std::string &flattened_name,
                                       unsigned int cycle) const;

        unsigned int getMaximumVersion(bool from_concrete_store, const std::string &flattened_name,
                                       unsigned int cycle) const;

        std::shared_ptr<Expression>
        reevaluateExpressionConcretely(const Manager &manager, Expression &expression,
                                       const std::map<std::string, std::shared_ptr<ConcreteExpression>>
                                               &contextualized_name_to_concrete_expression) const;

    private:
        const Configuration &_configuration;

        const Vertex *_vertex;

        std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                _contextualized_name_to_concrete_expression;

        std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
                _contextualized_name_to_symbolic_expression;

        std::map<std::string, std::shared_ptr<ShadowExpression>> _shadow_name_to_shadow_expression;

        // TODO (03.01.2022): Lower the path constraint to a combination of "whole-program" inputs or a combination
        //  of the least version of a variable?
        std::vector<std::shared_ptr<Expression>> _path_constraint;

        /// Data structures for Dimitri's encoding

        std::shared_ptr<AssumptionLiteralExpression> _assumption_literal;

        // keeps track of the topology-induced control-flow in form of assumption literals, maps an assumption
        // literal name (key) to the set of predecessor assumption literals (or/disjunction) that can reach the
        // vertex encoded by the key
        // TODO (29.01.2022): Refactor vector to set because ass lits are or-combined in this container
        std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                 VerificationConditionComparator>
                _assumption_literals;

        // maps an assumption literal name to the assumptions (resulting from assumes, e.g., if/while)
        std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>
                _assumptions;

        // maps an assumption literal name to the hard constraints (resulting from, e.g., assignments)
        std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator> _hard_constraints;

        // maps an assumption literal name at the intraprocedural call-to-return target of the caller to the assumption
        // literal name of the exit of the callee
        // TODO (30.01.2022): There can be more than one callee exit location at caller target, e.g., when no outputs
        //  are written and they are the last instructions of an if or while, hence, merging at the same join point
        std::map<std::string, std::string, VerificationConditionComparator>
                _unknown_over_approximating_summary_literals;

        // TODO (06.01.2022): I think explicit SymbolicVariables are not necessary, everything can be done via the
        //  contextualized name aka std::string.
        // std::map<std::string, std::shared_ptr<SymbolicVariable>> _contextualized_name_to_symbolic_variable;
    };
}// namespace se

#endif//AHORN_STATE_H
