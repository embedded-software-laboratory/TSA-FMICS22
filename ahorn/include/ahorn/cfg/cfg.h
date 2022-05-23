#ifndef AHORN_CFG_H
#define AHORN_CFG_H

#include <gtest/gtest_prod.h>

#include "cfg/edge.h"
#include "cfg/vertex.h"
#include "ir/interface.h"
#include "ir/variable.h"

#include "boost/iterator/filter_iterator.hpp"
#include "boost/iterator/indirect_iterator.hpp"
#include "boost/iterator/transform_iterator.hpp"

#include <map>
#include <memory>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

class TestLibCfg_Empty_If_Test;
class TestLibCfg_If_Test;
class TestLibCfg_If_Else_If_Test;
class TestLibCfg_Call_Test;

class Cfg : public ir::Name {
private:
    FRIEND_TEST(::TestLibCfg, Empty_If);
    FRIEND_TEST(::TestLibCfg, If);
    FRIEND_TEST(::TestLibCfg, If_Else_If);
    FRIEND_TEST(::TestLibCfg, Call);

public:
    using cfg_map_t = std::map<std::string, std::shared_ptr<Cfg>>;
    using cfg_ref_t = std::reference_wrapper<Cfg>;
    using const_cfg_ref_t = std::reference_wrapper<const Cfg>;
    using cfg_ref_map_t = std::map<std::string, cfg_ref_t>;
    // XXX vertex_t is shared, because of super-graph algorithms for program dependence graph, ifds, ...?
    using vertex_t = std::shared_ptr<Vertex>;
    using vertex_vec_t = std::vector<vertex_t>;
    using vertex_set_t = std::set<vertex_t, VertexLabelComparator>;
    using vertex_map_t = std::map<unsigned int, vertex_t>;
    using edge_t = std::shared_ptr<Edge>;
    using edge_vector_t = std::vector<edge_t>;

public:
    enum class Type { PROGRAM = 0, FUNCTION_BLOCK = 1, FUNCTION = 2 };

    // XXX default constructor disabled
    Cfg() = delete;
    // XXX copy constructor disabled
    Cfg(const Cfg &) = delete;
    // XXX copy assignment disabled
    Cfg &operator=(const Cfg &) = delete;

    // A cfg always has a name such that the variable names are bound to this environment.
    // XXX full constructor for building the cfg automatically
    Cfg(Type type, std::string name, std::unique_ptr<ir::Interface> interface, cfg_map_t name_to_cfg,
        vertex_map_t label_to_vertex, edge_vector_t edges, unsigned int entry_label, unsigned int exit_label);

private:
    struct getVertexReference : public std::unary_function<vertex_map_t::value_type, Vertex> {
        getVertexReference() = default;
        Vertex &operator()(const vertex_map_t::value_type &pair) const {
            return *(pair.second);
        }
    };

    using const_vertex_it = boost::transform_iterator<getVertexReference, vertex_map_t::const_iterator>;

public:
    const_vertex_it verticesBegin() const;
    const_vertex_it verticesEnd() const;

    /// Retrieves a type representative cfg given its type name.
    /// \param name of the type representative cfg
    /// \return a type representative cfg
    const Cfg &getCfg(const std::string &name) const;

    const ir::Interface &getInterface() const;

    Type getType() const;

    const Vertex &getEntry() const;
    unsigned int getEntryLabel() const;
    const Vertex &getExit() const;
    unsigned int getExitLabel() const;

    const Vertex &addVertex();

    /// Retrieves a vertex given the label. If the vertex does not exist in the cfg an exception is thrown.
    const Vertex &getVertex(unsigned int label) const;

    const Edge &addEdge(unsigned int source, unsigned int target, Edge::Type type = Edge::Type::INTRAPROCEDURAL);
    const Edge &getEdge(unsigned int source, unsigned int target) const;

    // Edge
    const Edge &getTrueEdge(unsigned int label) const;
    const Edge &getFalseEdge(unsigned int label) const;
    const Edge &getIntraproceduralEdge(unsigned int label) const;
    const Edge &getIntraproceduralCallToReturnEdge(unsigned int label) const;
    const Edge &getInterproceduralCallEdge(unsigned int label) const;
    const Edge &getInterproceduralReturnEdge(unsigned int label) const;

    std::shared_ptr<Cfg> getCallee(unsigned int label) const;

    std::vector<unsigned int> getCallLabels(const std::string &type_representative_name) const;

    const_vertex_it removeVertex(unsigned int label);

    // removes all incoming edges to the target label
    void removeIncomingEdges(unsigned int label);
    // removes all outgoing edges from the source label
    void removeOutgoingEdges(unsigned int label);

    std::vector<unsigned int> getPrecedingLabels(unsigned int label) const;

    std::vector<unsigned int> getSucceedingLabels(unsigned int label) const;

    edge_vector_t getOutgoingEdges(unsigned int label) const;
    edge_vector_t getIncomingEdges(unsigned int label) const;

    std::string toDot() const;

    std::string toDot(std::set<unsigned int> labels) const;

    // adds a variable to the underlying interface, updates the flattened interface, and notifies all users
    const ir::Variable &addVariable(std::shared_ptr<ir::Variable> variable);

    // removes a variable from the underlying interface, updates the flattened interface, and notifies all users
    void removeVariable(const std::string &name);

    std::shared_ptr<ir::Variable> getVariable(const ir::VariableReference &variable_reference) const;

private:
    void updateFlattenedInterface();

    // updates the vertices of the super-graph
    void updateFlattenedVertices();

private:
    struct getVariableReference
        : std::unary_function<std::map<std::string, std::shared_ptr<ir::Variable>>::value_type, ir::Variable> {
        getVariableReference() = default;
        ir::Variable &operator()(const std::map<std::string, std::shared_ptr<ir::Variable>>::value_type &p) const {
            return *(p.second);
        }
    };

private:
    using const_variable_it =
            boost::transform_iterator<getVariableReference,
                                      std::map<std::string, std::shared_ptr<ir::Variable>>::const_iterator>;

public:
    const_variable_it flattenedInterfaceBegin() const;
    const_variable_it flattenedInterfaceEnd() const;

private:
    void toDotSubgraph(std::stringstream &str, const Cfg &cfg, std::set<std::string> &visited_cfgs,
                       std::set<unsigned int> &labels) const;

private:
    struct getCfgReference : public std::unary_function<cfg_map_t::value_type, Cfg> {
        getCfgReference() = default;
        Cfg &operator()(const cfg_map_t::value_type &pair) const {
            return *(pair.second);
        }
    };

    struct getConstCfgReference : public std::unary_function<cfg_ref_map_t::value_type, Cfg> {
        getConstCfgReference() = default;
        const Cfg &operator()(const cfg_ref_map_t::value_type &pair) const {
            return pair.second.get();
        }
    };

public:
    using const_cfg_it = boost::transform_iterator<getCfgReference, cfg_map_t::const_iterator>;
    using const_cfg_ref_it = boost::transform_iterator<getConstCfgReference, cfg_ref_map_t::const_iterator>;

    const_cfg_it calleesBegin() const;
    const_cfg_it calleesEnd() const;
    const_cfg_ref_it callersBegin() const;
    const_cfg_ref_it callersEnd() const;

    std::vector<const_cfg_ref_t> getFlattenedCallees() const;

private:
    struct isIntraproceduralEdge {
        bool operator()(const Edge &edge) {
            return (edge.getType() != Edge::Type::INTERPROCEDURAL_CALL) &&
                   (edge.getType() != Edge::Type::INTERPROCEDURAL_RETURN);
        }
    };

public:
    using const_edge_it = boost::indirect_iterator<edge_vector_t::const_iterator>;
    using const_intraprocedural_edge_it = boost::filter_iterator<isIntraproceduralEdge, const_edge_it>;

    // iterator to all edges in the cfg
    const_edge_it edgesBegin() const;
    const_edge_it edgesEnd() const;

    // filter iterator for all intraprocedural edges
    const_intraprocedural_edge_it intraproceduralEdgesBegin() const;
    const_intraprocedural_edge_it intraproceduralEdgesEnd() const;

private:
    const Type _type;
    std::unique_ptr<ir::Interface> _interface;
    std::map<std::string, std::shared_ptr<ir::Variable>> _name_to_flattened_variable;
    cfg_map_t _name_to_cfg;
    vertex_map_t _label_to_vertex;
    edge_vector_t _edges;
    unsigned int _entry_label;
    unsigned int _exit_label;

    // maps fully qualified names of a caller/callee cfg to its respective type representative cfg
    cfg_map_t _name_to_callee_cfg;
    cfg_ref_map_t _name_to_caller_cfg;

    // super-graph
    // N*
    vertex_map_t _label_to_flattened_vertex;
    // vector of all call-site vertices occurring in this cfg and its callees
    vertex_set_t _call_sites;
    // vector of all return-site vertices occurring in this cfg and its callees
    vertex_set_t _return_sites;
    // vector of all entry vertices occurring in this cfg and its callees
    vertex_set_t _entries;
    // vector of all exit vertices occurring in this cfg and its callees
    vertex_set_t _exits;
};

#endif//AHORN_CFG_H
