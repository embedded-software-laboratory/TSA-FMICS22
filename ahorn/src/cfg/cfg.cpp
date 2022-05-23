#include "cfg/cfg.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/if_instruction.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <spdlog/spdlog.h>
#include <sstream>

using namespace ir;

Cfg::Cfg(Type type, std::string name, std::unique_ptr<Interface> interface, cfg_map_t name_to_cfg,
         vertex_map_t label_to_vertex, edge_vector_t edges, unsigned int entry_label, unsigned int exit_label)
    : Name(std::move(name)), _type(type), _interface(std::move(interface)), _name_to_cfg(std::move(name_to_cfg)),
      _label_to_vertex(std::move(label_to_vertex)), _edges(std::move(edges)), _entry_label(entry_label),
      _exit_label(exit_label) {
    for (auto it = _interface->variablesBegin(); it != _interface->variablesEnd(); ++it) {
        it->setParent(*this);
    }
    updateFlattenedInterface();
    // build callee/caller relation
    for (const auto &p : _name_to_cfg) {
        _name_to_callee_cfg.emplace(p.first, p.second);
        p.second->_name_to_caller_cfg.emplace(getFullyQualifiedName(), *this);
    }
    // TODO: build super-graph members
    updateFlattenedVertices();
}

const Vertex &Cfg::getEntry() const {
    return *_label_to_vertex.at(_entry_label);
}

const Vertex &Cfg::getExit() const {
    return *_label_to_vertex.at(_exit_label);
}

std::string Cfg::toDot() const {
    std::stringstream str;
    // start graph
    str << "digraph ";
    std::string name = getFullyQualifiedName();
    std::replace(name.begin(), name.end(), '.', '_');
    str << name << " {\n";
    std::set<std::string> visited_cfg;
    std::set<unsigned int> labels;
    toDotSubgraph(str, *this, visited_cfg, labels);
    // end graph
    str << "}";
    return str.str();
}

void Cfg::toDotSubgraph(std::stringstream &str, const Cfg &cfg, std::set<std::string> &visited_cfg,
                        std::set<unsigned int> &labels) const {
    std::string name = cfg.getName();
    assert(visited_cfg.count(name) == 0);
    visited_cfg.insert(name);

    // handle nested graphs
    for (auto it = cfg.calleesBegin(); it != cfg.calleesEnd(); ++it) {
        if (visited_cfg.count(it->getName()) == 0) {
            toDotSubgraph(str, *it, visited_cfg, labels);// recurse
        }
    }

    // start subgraph
    str << "subgraph cluster_" << name << "{\n";
    str << "label=\"" << name << "\";\n";

    // vertices
    for (const auto &label_to_vertex : cfg._label_to_vertex) {
        unsigned int label = label_to_vertex.first;
        const std::shared_ptr<Vertex> &vertex = label_to_vertex.second;
        bool ranked = false;
        if (cfg._entry_label == label) {
            str << "{rank=source; ";
            ranked = true;
        } else if (cfg._exit_label == label) {
            str << "{rank=sink; ";
            ranked = true;
        }
        str << std::to_string(label);
        // attributes
        str << "[";
        if (cfg._entry_label == label) {
            if (cfg._label_to_vertex.at(cfg._entry_label)->getInstruction() == nullptr) {
                str << "label=\""
                    << "L" << std::to_string(cfg._entry_label) << ":\n"
                    << "ENTRY"
                    << "\"";
                str << ",shape=doublecircle";
            } else {
                str << "label=\""
                    << "L" << std::to_string(cfg._entry_label) << ":\n";
                vertex->getInstruction()->print(str);
                str << "\"";
                str << ",shape=doublecircle";
            }
        } else if (cfg._exit_label == label) {
            if (cfg._label_to_vertex.at(cfg._exit_label)->getInstruction() == nullptr) {
                str << "label=\""
                    << "L" << std::to_string(cfg._exit_label) << ":\n"
                    << "EXIT"
                    << "\"";
                str << ",shape=doublecircle";
            } else {
                str << "label=\""
                    << "L" << std::to_string(cfg._exit_label) << ":\n";
                vertex->getInstruction()->print(str);
                str << "\"";
                str << ",shape=doublecircle";
            }
        } else {
            str << "label=\"";
            str << "L" << std::to_string(label) << ":\n";
            if (vertex->getInstruction()) {
                vertex->getInstruction()->print(str);
            }
            str << "\"";
            /*
            if (vertex->getType() == Vertex::Type::CHANGE_ANNOTATED) {
                str << ",color=green";
            } else if (labels.count(label)) {
                str << ",color=red";
            }
            */
        }
        str << "]";
        if (ranked) {
            str << "}";
        }
        str << "\n";
    }
    // edges
    for (const auto &edge : cfg._edges) {
        unsigned int source_label = edge->getSourceLabel();
        unsigned int target_label = edge->getTargetLabel();
        if (edge->getType() == Edge::Type::INTERPROCEDURAL_CALL) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[color=blue]";
            str << ";\n";
        } else if (edge->getType() == Edge::Type::INTERPROCEDURAL_RETURN) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[color=blue,style=dashed]";
            str << ";\n";
            // XXX return-to-call label relation
            str << std::to_string(source_label) << " -> " << std::to_string(edge->getCallLabel());
            // attributes
            str << "[color=orange,style=dashed,";
            str << "label=\"" << edge->getCallerName() << "\""
                << "]";
            str << ";\n";
        } else if (edge->getType() == Edge::Type::INTRAPROCEDURAL) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[]";
            str << ";\n";
        } else if (edge->getType() == Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[color=grey,style=dashed]";
            str << ";\n";
        } else if (edge->getType() == Edge::Type::TRUE_BRANCH) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[color=green]";
            str << ";\n";
        } else if (edge->getType() == Edge::Type::FALSE_BRANCH) {
            str << std::to_string(source_label) << " -> " << std::to_string(target_label);
            // attributes
            str << "[color=red]";
            str << ";\n";
        }
    }
    // end subgraph
    str << "}\n";
}

Cfg::const_vertex_it Cfg::verticesBegin() const {
    return boost::make_transform_iterator(_label_to_vertex.begin(), getVertexReference());
}

Cfg::const_vertex_it Cfg::verticesEnd() const {
    return boost::make_transform_iterator(_label_to_vertex.end(), getVertexReference());
}

Cfg::edge_vector_t Cfg::getOutgoingEdges(unsigned int label) const {
    edge_vector_t outgoing_edges;
    for (const auto &edge : _edges) {
        if (edge->getSourceLabel() == label) {
            outgoing_edges.emplace_back(edge);
        }
    }
    return outgoing_edges;
}

Cfg::edge_vector_t Cfg::getIncomingEdges(unsigned int label) const {
    edge_vector_t incoming_edges;
    for (const auto &edge : _edges) {
        if (edge->getTargetLabel() == label) {
            incoming_edges.emplace_back(edge);
        }
    }
    return incoming_edges;
}

std::vector<unsigned int> Cfg::getPrecedingLabels(unsigned int label) const {
    std::vector<unsigned int> preceding_labels;
    for (const auto &edge : _edges) {
        if (edge->getTargetLabel() == label) {
            preceding_labels.push_back(edge->getSourceLabel());
        }
    }
    return preceding_labels;
}

std::vector<unsigned int> Cfg::getSucceedingLabels(unsigned int label) const {
    std::vector<unsigned int> succeeding_labels;
    for (const auto &edge : _edges) {
        if (edge->getSourceLabel() == label) {
            succeeding_labels.push_back(edge->getTargetLabel());
        }
    }
    return succeeding_labels;
}

Cfg::const_vertex_it Cfg::removeVertex(unsigned int label) {
    auto it = _label_to_vertex.find(label);
    if (it != _label_to_vertex.end()) {
        return boost::make_transform_iterator(_label_to_vertex.erase(it), getVertexReference());
    } else {
        throw std::logic_error("Vertex not found.");
    }
}

void Cfg::removeIncomingEdges(unsigned int label) {
    _edges.erase(std::remove_if(_edges.begin(), _edges.end(),
                                [label](const auto &edge) { return edge->getTargetLabel() == label; }),
                 _edges.end());
}

void Cfg::removeOutgoingEdges(unsigned int label) {
    _edges.erase(std::remove_if(_edges.begin(), _edges.end(),
                                [label](const auto &edge) { return edge->getSourceLabel() == label; }),
                 _edges.end());
}

unsigned int Cfg::getEntryLabel() const {
    return _entry_label;
}

unsigned int Cfg::getExitLabel() const {
    return _exit_label;
}

const Vertex &Cfg::getVertex(unsigned int label) const {
    auto it = std::find_if(_label_to_vertex.begin(), _label_to_vertex.end(),
                           [label](auto &p) { return p.first == label; });
    if (it != _label_to_vertex.end()) {
        return *it->second;
    } else {
        throw std::logic_error("No vertex with label " + std::to_string(label) + " exists.");
    }
}

Cfg::const_cfg_it Cfg::calleesBegin() const {
    return boost::make_transform_iterator(_name_to_callee_cfg.begin(), getCfgReference());
}

Cfg::const_cfg_it Cfg::calleesEnd() const {
    return boost::make_transform_iterator(_name_to_callee_cfg.end(), getCfgReference());
}

Cfg::const_cfg_ref_it Cfg::callersBegin() const {
    return boost::make_transform_iterator(_name_to_caller_cfg.begin(), getConstCfgReference());
}

Cfg::const_cfg_ref_it Cfg::callersEnd() const {
    return boost::make_transform_iterator(_name_to_caller_cfg.end(), getConstCfgReference());
}

std::string Cfg::toDot(std::set<unsigned int> labels) const {
    std::stringstream str;
    // start graph
    str << "digraph ";
    std::string name = getFullyQualifiedName();
    std::replace(name.begin(), name.end(), '.', '_');
    str << name << " {\n";
    std::set<std::string> visited_cfg;
    toDotSubgraph(str, *this, visited_cfg, labels);
    // end graph
    str << "}";
    return str.str();
}

const Edge &Cfg::getEdge(unsigned int source, unsigned int target) const {
    auto it = std::find_if(_edges.begin(), _edges.end(), [source, target](const auto &e) {
        return e->getSourceLabel() == source && e->getTargetLabel() == target;
    });
    if (it == _edges.end()) {
        throw std::logic_error("Edge does not exist.");
    } else {
        return **it;
    }
}

const Interface &Cfg::getInterface() const {
    return *_interface;
}

Cfg::Type Cfg::getType() const {
    return _type;
}

const Edge &Cfg::addEdge(unsigned int source, unsigned int target, Edge::Type type) {
    _edges.push_back(std::make_unique<Edge>(source, target, type));
    return *_edges.back();
}

void Cfg::updateFlattenedInterface() {
    _name_to_flattened_variable.clear();
    // flattens the interface of a type representative module resulting from a compiled pou, i.e., creates new
    // variables for nested variables by replacing the fully qualified name with the corresponding prefix
    for (auto variable = _interface->variablesBegin(); variable != _interface->variablesEnd(); ++variable) {
        if (variable->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // XXX Variable : DataType, e.g., f : Fb1
            std::string variable_name = variable->getName();
            // XXX Find the type representative module for the variable
            std::string cfg_type_name = variable->getDataType().getFullyQualifiedName();
            const Cfg &type_representative_cfg = *_name_to_cfg.at(cfg_type_name);
            // XXX Replace prefix with variable name
            // XXX Fb1.x, Fb1.Fb2.y, ... --> f.x, f.g.y, ...
            for (auto it = type_representative_cfg.flattenedInterfaceBegin();
                 it != type_representative_cfg.flattenedInterfaceEnd(); ++it) {
                std::string fully_qualified_name = it->getFullyQualifiedName();
                std::string flattened_variable_name =
                        fully_qualified_name.replace(0, cfg_type_name.size(), variable_name);
                auto flattened_variable = std::make_unique<Variable>(std::move(flattened_variable_name),
                                                                     it->getDataType().clone(), it->getStorageType());
                flattened_variable->setParent(*this);
                _name_to_flattened_variable.emplace(flattened_variable->getName(), std::move(flattened_variable));
            }
        } else {
            std::unique_ptr<Variable> flattened_variable;
            if (variable->hasInitialization()) {
                flattened_variable =
                        std::make_unique<Variable>(variable->getName(), variable->getDataType().clone(),
                                                   variable->getStorageType(), variable->getInitialization().clone());
            } else {
                flattened_variable = std::make_unique<Variable>(variable->getName(), variable->getDataType().clone(),
                                                                variable->getStorageType());
            }
            flattened_variable->setParent(*this);
            _name_to_flattened_variable.emplace(flattened_variable->getName(), std::move(flattened_variable));
        }
    }
}

Cfg::const_variable_it Cfg::flattenedInterfaceBegin() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.begin(), Cfg::getVariableReference());
}

Cfg::const_variable_it Cfg::flattenedInterfaceEnd() const {
    return boost::make_transform_iterator(_name_to_flattened_variable.end(), Cfg::getVariableReference());
}

// TODO: remove this function as it is redundant; each variable reference carries its own variable?
std::shared_ptr<Variable> Cfg::getVariable(const VariableReference &variable_reference) const {
    if (auto *variable_access = dynamic_cast<const VariableAccess *>(&variable_reference)) {
        return _interface->getVariable(variable_access->getVariable()->getName());
    } else if (auto *field_access = dynamic_cast<const FieldAccess *>(&variable_reference)) {
        std::stringstream name;
        field_access->print(name);
        assert(_name_to_flattened_variable.count(name.str()));
        return _name_to_flattened_variable.at(name.str());
    }
    throw std::logic_error("Invalid, this should not be reachable.");
}

const Variable &Cfg::addVariable(std::shared_ptr<Variable> variable) {
    variable->setParent(*this);
    const Variable &added_variable = _interface->addVariable(std::move(variable));
    updateFlattenedInterface();
    // recurse on the callers, i.e., "users" of this interface
    for (auto &name_to_caller_cfg : _name_to_caller_cfg) {
        name_to_caller_cfg.second.get().updateFlattenedInterface();
    }
    return added_variable;
}

const Vertex &Cfg::addVertex() {
    int next_free_label = -1;
    for (auto &label_to_vertex : _label_to_vertex) {
        if (label_to_vertex.first >= next_free_label) {
            next_free_label = label_to_vertex.first + 1;
        }
    }
    for (auto it = calleesBegin(); it != calleesEnd(); ++it) {
        if (it->getExitLabel() >= next_free_label) {
            next_free_label = it->getExitLabel() + 1;
        }
    }
    for (auto it = callersBegin(); it != callersBegin(); ++it) {
        if (it->getExitLabel() >= next_free_label) {
            next_free_label = it->getExitLabel() + 1;
        }
    }
    assert(next_free_label != -1);
    auto it = _label_to_vertex.emplace(next_free_label, std::make_shared<Vertex>(next_free_label));
    return *(it.first->second);
}

void Cfg::removeVariable(const std::string &name) {
    _interface->removeVariable(name);
    updateFlattenedInterface();
    // recurse on the callers, i.e., "users" of this interface
    for (auto &name_to_caller_cfg : _name_to_caller_cfg) {
        name_to_caller_cfg.second.get().updateFlattenedInterface();
    }
}

Cfg::const_edge_it Cfg::edgesBegin() const {
    return boost::make_indirect_iterator(_edges.begin());
}

Cfg::const_edge_it Cfg::edgesEnd() const {
    return boost::make_indirect_iterator(_edges.end());
}

Cfg::const_intraprocedural_edge_it Cfg::intraproceduralEdgesBegin() const {
    return boost::make_filter_iterator<Cfg::isIntraproceduralEdge>(edgesBegin(), edgesEnd());
}

Cfg::const_intraprocedural_edge_it Cfg::intraproceduralEdgesEnd() const {
    return boost::make_filter_iterator<Cfg::isIntraproceduralEdge>(edgesEnd(), edgesEnd());
}

void Cfg::updateFlattenedVertices() {
    // TODO
    _label_to_flattened_vertex.clear();
    for (auto &kv : _label_to_vertex) {
        if (kv.first == _entry_label) {
            _entries.emplace(kv.second);
        }
        if (kv.first == _exit_label) {
            _exits.emplace(kv.second);
        }
        _label_to_flattened_vertex.emplace(kv.first, kv.second);
    }
    for (auto it = calleesBegin(); it != calleesEnd(); ++it) {
        for (auto &kv : it->_label_to_vertex) {
            if (kv.first == _entry_label) {
                _entries.emplace(kv.second);
            }
            if (kv.first == _exit_label) {
                _exits.emplace(kv.second);
            }
            _label_to_flattened_vertex.emplace(kv.first, kv.second);
        }
    }
}

std::vector<Cfg::const_cfg_ref_t> Cfg::getFlattenedCallees() const {
    std::vector<const_cfg_ref_t> flattened_callees;
    for (auto it = calleesBegin(); it != calleesEnd(); ++it) {
        flattened_callees.emplace_back(*it);
        for (const auto &callee : it->getFlattenedCallees()) {
            auto find_it = std::find_if(
                    std::begin(flattened_callees), std::end(flattened_callees), [callee](const auto &flattened_callee) {
                        return callee.get().getFullyQualifiedName() == flattened_callee.get().getFullyQualifiedName();
                    });
            if (find_it == std::end(flattened_callees)) {
                flattened_callees.push_back(*find_it);
            }
        }
    }
    return flattened_callees;
}

const Edge &Cfg::getTrueEdge(unsigned int label) const {
    assert(_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::IF ||
           _label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::WHILE);
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getSourceLabel() == label && edge->getType() == Edge::Type::TRUE_BRANCH;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No true edge for if instruction at label " + std::to_string(label) + " exists.");
    }
}

const Edge &Cfg::getFalseEdge(unsigned int label) const {
    assert(_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::IF ||
           _label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::WHILE);
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getSourceLabel() == label && edge->getType() == Edge::Type::FALSE_BRANCH;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No false edge for if instruction at label " + std::to_string(label) + " exists.");
    }
}

const Edge &Cfg::getIntraproceduralEdge(unsigned int label) const {
    assert(_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::ASSIGNMENT ||
           _label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::HAVOC);
    auto succeeding_labels = getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getSourceLabel() == label && edge->getType() == Edge::Type::INTRAPROCEDURAL;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No intraprocedural edge for instruction at label " + std::to_string(label) +
                               " exists.");
    }
}

const Edge &Cfg::getIntraproceduralCallToReturnEdge(unsigned int label) const {
    assert(_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::CALL ||
           (_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::SEQUENCE));
    auto succeeding_labels = getSucceedingLabels(label);
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getSourceLabel() == label && edge->getType() == Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No intraprocedural call-to-return edge for instruction at label " +
                               std::to_string(label) + " exists.");
    }
}

const Edge &Cfg::getInterproceduralCallEdge(unsigned int label) const {
    assert(_label_to_vertex.at(label)->getInstruction()->getKind() == Instruction::Kind::CALL);
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getSourceLabel() == label && edge->getType() == Edge::Type::INTERPROCEDURAL_CALL;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No interprocedural edge for call instruction at label " + std::to_string(label) +
                               " exists.");
    }
}

const Edge &Cfg::getInterproceduralReturnEdge(unsigned int label) const {
    auto it = std::find_if(_edges.begin(), _edges.end(), [label](const auto &edge) {
        return edge->getTargetLabel() == label && edge->getType() == Edge::Type::INTERPROCEDURAL_RETURN;
    });
    if (it != _edges.end()) {
        return **it;
    } else {
        throw std::logic_error("No interprocedural return edge for instruction at label " + std::to_string(label) +
                               " exists.");
    }
}

const Cfg &Cfg::getCfg(const std::string &name) const {
    if (_name_to_cfg.count(name)) {
        return *_name_to_cfg.at(name);
    } else {
        throw std::logic_error("No type representative cfg for " + name + " exists.");
    }
}

std::vector<unsigned int> Cfg::getCallLabels(const std::string &type_representative_name) const {
    std::vector<unsigned int> call_labels;
    std::shared_ptr<Cfg> callee = _name_to_callee_cfg.at(type_representative_name);
    for (const auto &edge : _edges) {
        if (edge->getType() == Edge::Type::INTERPROCEDURAL_CALL) {
            if (edge->getTargetLabel() == callee->getEntryLabel()) {
                call_labels.push_back(edge->getSourceLabel());
            }
        }
    }
    return call_labels;
}

std::shared_ptr<Cfg> Cfg::getCallee(unsigned int label) const {
    const Edge &interprocedural_call_edge = getInterproceduralCallEdge(label);
    unsigned int entry_label = interprocedural_call_edge.getTargetLabel();
    auto it = std::find_if(_name_to_callee_cfg.begin(), _name_to_callee_cfg.end(),
                           [entry_label](const auto &p) { return entry_label == p.second->getEntryLabel(); });
    if (it == _name_to_callee_cfg.end()) {
        throw std::runtime_error("No callee for label exists.");
    } else {
        return it->second;
    }
}