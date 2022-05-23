#ifndef AHORN_SIMULATOR_STATE_H
#define AHORN_SIMULATOR_STATE_H

#include "cfg/vertex.h"

#include "z3++.h"

#include <map>

namespace se::simulator {
    class State {
    public:
        // XXX default constructor disabled
        State() = delete;
        // XXX copy constructor disabled
        State(const State &other) = delete;
        // XXX copy assignment disabled
        State &operator=(const State &) = delete;

        State(const Vertex &vertex, std::map<std::string, z3::expr> concrete_valuations);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const State &state) {
            return state.print(os);
        }

        const Vertex &getVertex() const;

        void setVertex(const Vertex &vertex);

        z3::expr getConcreteValuation(const std::string &contextualized_name) const;

        void setConcreteValuation(const std::string &contextualized_name, const z3::expr &valuation);

        void updateConcreteValuation(const std::string &contextualized_name, const z3::expr &valuation);

    private:
        const Vertex *_vertex;
        std::map<std::string, z3::expr> _concrete_valuations;
    };
}// namespace se::simulator

#endif//AHORN_SIMULATOR_STATE_H
