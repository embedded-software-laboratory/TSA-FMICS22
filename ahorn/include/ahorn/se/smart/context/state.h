#ifndef AHORN_SMART_STATE_H
#define AHORN_SMART_STATE_H

#include "cfg/vertex.h"

#include "z3++.h"

#include <map>
#include <memory>

namespace se::smart {
    class State {
    public:
        // bookkeeping of conditionals and their corresponding valuations
        using store_t = std::map<unsigned int, std::map<std::string, z3::expr>>;

        // XXX default constructor disabled
        State() = delete;
        // XXX copy constructor disabled
        State(const State &other) = delete;
        // XXX copy assignment disabled
        State &operator=(const State &) = delete;

        State(const Vertex &vertex, store_t concrete_valuations, store_t symbolic_valuations);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const State &state) {
            return state.print(os);
        }

        const Vertex &getVertex() const;

        void setVertex(const Vertex &vertex);

        z3::expr getConcreteValuation(const std::string &contextualized_name) const;

        void setConcreteValuation(unsigned int k, const std::string &contextualized_name, const z3::expr &valuation);

        z3::expr getSymbolicValuation(const std::string &contextualized_name) const;

        void setSymbolicValuation(unsigned int k, const std::string &contextualized_name, const z3::expr &valuation);

        const store_t &getSymbolicValuations() const;

        const std::vector<z3::expr> &getPathConstraint() const;

        void pushPathConstraint(const z3::expr &constraint);

        void backtrack(const Vertex &vertex, int j, z3::model &model, std::vector<z3::expr> path_constraint);

        unsigned int getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                       bool from_concrete_valuations = true) const;

    private:
        unsigned int getHighestVersion(const std::string &flattened_name, unsigned int cycle,
                                       const store_t &valuations) const;

    private:
        const Vertex *_vertex;
        store_t _concrete_valuations;
        store_t _symbolic_valuations;
        std::vector<z3::expr> _path_constraint;
    };
}// namespace se::smart

#endif//AHORN_SMART_STATE_H
