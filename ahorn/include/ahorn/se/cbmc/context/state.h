#ifndef AHORN_CBMC_STATE_H
#define AHORN_CBMC_STATE_H

#include "cfg/vertex.h"

#include "z3++.h"

#include <map>
#include <vector>

namespace se::cbmc {
    class State {
    public:
        // XXX default constructor disabled
        State() = delete;
        // XXX copy constructor disabled
        State(const State &other) = delete;
        // XXX copy assignment disabled
        State &operator=(const State &) = delete;

        State(std::map<std::string, z3::expr> initial_valuations,
              std::map<std::string, unsigned int> flattened_name_to_version, const Vertex &vertex,
              z3::expr assumption_literal, std::map<std::string, std::vector<z3::expr>> assumption_literals,
              std::map<std::string, std::vector<z3::expr>> assumptions,
              std::map<std::string, std::map<std::string, z3::expr>> hard_constraints);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const State &state) {
            return state.print(os);
        }

        const std::map<std::string, z3::expr> &getInitialValuations() const;

        const std::map<std::string, unsigned int> &getLocalVersioning() const;

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        const Vertex &getVertex() const;

        void setVertex(const Vertex &vertex);

        z3::expr getAssumptionLiteral() const;

        void setAssumptionLiteral(z3::expr assumption_literal);

        const std::map<std::string, std::vector<z3::expr>> &getAssumptionLiterals() const;

        void pushAssumptionLiteral(std::string assumption_literal_name, z3::expr assumption_literal);

        const std::map<std::string, std::vector<z3::expr>> &getAssumptions() const;

        void pushAssumption(std::string assumption_literal_name, z3::expr assumption);

        const std::map<std::string, std::map<std::string, z3::expr>> &getHardConstraints() const;

        void pushHardConstraint(std::string assumption_literal_name, const std::string &contextualized_name,
                                z3::expr expression);

        std::unique_ptr<State> fork(const Vertex &vertex, z3::expr next_assumption_literal,
                                    const std::string &next_assumption_literal_name, z3::expr encoded_expression);

    private:
        // initial valuations
        std::map<std::string, z3::expr> _initial_valuations;

        // "locally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;

        const Vertex *_vertex;
        z3::expr _assumption_literal;
        // keeps track of the topology-induced control-flow in form of assumption literals, maps an assumption
        // literal name to the set of predecessor assumption literals (or/disjunction) that can reach the encoded vertex
        std::map<std::string, std::vector<z3::expr>> _assumption_literals;
        std::map<std::string, std::vector<z3::expr>> _assumptions;
        std::map<std::string, std::map<std::string, z3::expr>> _hard_constraints;
    };
}// namespace se::cbmc

#endif//AHORN_CBMC_STATE_H
