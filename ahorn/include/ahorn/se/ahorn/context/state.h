#ifndef AHORN_AHORN_STATE_H
#define AHORN_AHORN_STATE_H

#include "cfg/vertex.h"
#include "se/ahorn/z3/solver.h"

#include "z3++.h"

#include <map>

namespace se::ahorn {
    class State {
    private:
        friend class Executor;

    public:
        // XXX default constructor disabled
        State() = delete;
        // XXX copy constructor disabled
        State(const State &other) = delete;
        // XXX copy assignment disabled
        State &operator=(const State &) = delete;

        State(const Vertex &vertex, std::map<std::string, z3::expr> concrete_valuations,
              std::map<std::string, z3::expr> symbolic_valuations, std::vector<z3::expr> path_constraint,
              std::map<std::string, unsigned int> flattened_name_to_version);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const State &state) {
            return state.print(os);
        }

        const Vertex &getVertex() const;

        void setVertex(const Vertex &vertex);

        const std::map<std::string, z3::expr> &getConcreteValuations() const;

        z3::expr getConcreteValuation(const std::string &contextualized_name) const;

        void setConcreteValuation(const std::string &contextualized_name, const z3::expr &valuation);

        const std::map<std::string, z3::expr> &getSymbolicValuations() const;

        z3::expr getSymbolicValuation(const std::string &contextualized_name) const;

        void setSymbolicValuation(const std::string &contextualized_name, const z3::expr &valuation);

        const std::vector<z3::expr> &getPathConstraint() const;

        void pushPathConstraint(const z3::expr &expression);

        void setVersion(const std::string &flattened_name, unsigned int version);

        std::unique_ptr<State> fork(Solver &solver, const Vertex &vertex, z3::model &model,
                                    const z3::expr &expression) const;

        unsigned int getLowestVersion(const std::string &flattened_name, unsigned int cycle,
                                      bool from_concrete_valuations = true) const;

        unsigned int getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                       bool from_concrete_valuations = true) const;

    private:
        unsigned int getLowestVersion(const std::string &flattened_name, unsigned int cycle,
                                      const std::map<std::string, z3::expr> &valuations) const;

        unsigned int getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                       const std::map<std::string, z3::expr> &valuations) const;

    private:
        const Vertex *_vertex;
        std::map<std::string, z3::expr> _concrete_valuations;
        std::map<std::string, z3::expr> _symbolic_valuations;
        std::vector<z3::expr> _path_constraint;

        // memoization of the highest version of a concrete and symbolic variable (stores are invariant with respect
        // to versioning) for quick look-up
        std::map<std::string, unsigned int> _flattened_name_to_version;
    };
}// namespace se::ahorn

#endif//AHORN_AHORN_STATE_H
