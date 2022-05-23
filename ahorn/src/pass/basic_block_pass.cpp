#include "pass/basic_block_pass.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/sequence_instruction.h"

using namespace pass;

std::shared_ptr<Cfg> BasicBlockPass::apply(const Cfg &cfg) {
    std::vector<std::shared_ptr<ir::Variable>> variables;
    for (auto it = cfg.getInterface().variablesBegin(); it != cfg.getInterface().variablesEnd(); ++it) {
        if (it->hasInitialization()) {
            variables.push_back(std::make_shared<ir::Variable>(it->getName(), it->getDataType().clone(),
                                                               it->getStorageType(), it->getInitialization().clone()));
        } else {
            variables.push_back(
                    std::make_shared<ir::Variable>(it->getName(), it->getDataType().clone(), it->getStorageType()));
        }
    }
    std::unique_ptr<ir::Interface> interface = std::make_unique<ir::Interface>(std::move(variables));
    std::map<std::string, std::shared_ptr<Cfg>> type_representative_name_to_cfg;
    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        std::shared_ptr<Cfg> callee_cfg = apply(*callee_it);
        type_representative_name_to_cfg.emplace(callee_cfg->getName(), std::move(callee_cfg));
    }
    // determine leaders
    std::set<unsigned int> leaders;
    for (auto vertex_it = cfg.verticesBegin(); vertex_it != cfg.verticesEnd(); ++vertex_it) {
        unsigned int label = vertex_it->getLabel();
        // a vertex is always leading in case it is a merge point
        if (cfg.getPrecedingLabels(label).size() > 1) {
            leaders.insert(label);
        }
        switch (vertex_it->getType()) {
            case Vertex::Type::ENTRY: {
                leaders.insert(label);
                std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
                assert(succeeding_labels.size() == 1);
                leaders.insert(succeeding_labels.at(0));
                break;
            }
            case Vertex::Type::REGULAR: {
                ir::Instruction *instruction = vertex_it->getInstruction();
                assert(instruction != nullptr);
                switch (instruction->getKind()) {
                    case ir::Instruction::Kind::ASSIGNMENT: {
                        break;
                    }
                    case ir::Instruction::Kind::CALL: {
                        const Edge &intraprocedural_call_to_return_edge = cfg.getIntraproceduralCallToReturnEdge(label);
                        leaders.insert(intraprocedural_call_to_return_edge.getTargetLabel());
                        break;
                    }
                    case ir::Instruction::Kind::GOTO:
                        break;
                    case ir::Instruction::Kind::IF: {
                        std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
                        assert(succeeding_labels.size() == 2);
                        for (unsigned int succeeding_label : succeeding_labels) {
                            leaders.insert(succeeding_label);
                        }
                        break;
                    }
                    case ir::Instruction::Kind::WHILE: {
                        // XXX in contrast to if, the while header is always a basic block
                        leaders.insert(label);
                        std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
                        assert(succeeding_labels.size() == 2);
                        for (unsigned int succeeding_label : succeeding_labels) {
                            leaders.insert(succeeding_label);
                        }
                        break;
                    }
                    case ir::Instruction::Kind::HAVOC: {
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid instruction kind.");
                }
                break;
            }
            case Vertex::Type::EXIT: {
                leaders.insert(label);
                break;
            }
            default:
                throw std::runtime_error("Invalid vertex type.");
        }
    }
    // build basic blocks and edge relation
    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    std::vector<std::shared_ptr<Edge>> edges;
    for (unsigned int leader : leaders) {
        if (leader == cfg.getEntryLabel()) {
            label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, Vertex::Type::ENTRY));
            std::vector<std::shared_ptr<Edge>> outgoing_edges = cfg.getOutgoingEdges(leader);
            assert(outgoing_edges.size() == 1);
            edges.push_back(std::make_shared<Edge>(outgoing_edges.at(0)->getSourceLabel(),
                                                   outgoing_edges.at(0)->getTargetLabel(),
                                                   outgoing_edges.at(0)->getType()));
        } else if (leader == cfg.getExitLabel()) {
            label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, Vertex::Type::EXIT));
            assert(cfg.getOutgoingEdges(leader).empty());
        } else {
            unsigned int label = leader;
            const Vertex *vertex = &cfg.getVertex(leader);
            assert(vertex->getType() == Vertex::Type::REGULAR);
            ir::Instruction *instruction = vertex->getInstruction();
            assert(instruction != nullptr);
            switch (instruction->getKind()) {
                case ir::Instruction::Kind::HAVOC: {
                    // XXX fall-through, handle as ASSIGNMENT
                }
                case ir::Instruction::Kind::ASSIGNMENT: {
                    std::vector<unsigned int> labels{label};
                    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
                    do {
                        assert(succeeding_labels.size() == 1);
                        label = succeeding_labels.at(0);
                        if (leaders.find(label) == leaders.end()) {
                            vertex = &cfg.getVertex(label);
                            assert(vertex->getType() == Vertex::Type::REGULAR);
                            instruction = vertex->getInstruction();
                            assert(instruction != nullptr);
                            if (instruction->getKind() == ir::Instruction::Kind::IF ||
                                instruction->getKind() == ir::Instruction::Kind::CALL) {
                                labels.push_back(label);
                                break;
                            } else if (instruction->getKind() == ir::Instruction::Kind::WHILE ||
                                       instruction->getKind() == ir::Instruction::Kind::GOTO ||
                                       instruction->getKind() == ir::Instruction::Kind::SEQUENCE) {
                                throw std::logic_error("Not implemented yet.");
                            } else {
                                labels.push_back(label);
                                succeeding_labels = cfg.getSucceedingLabels(label);
                            }
                        } else {
                            break;
                        }
                    } while (true);
                    if (labels.size() == 1) {
                        vertex = &cfg.getVertex(leader);
                        assert(vertex->getType() == Vertex::Type::REGULAR);
                        instruction = vertex->getInstruction();
                        assert(instruction != nullptr);
                        label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, instruction->clone()));
                        std::vector<std::shared_ptr<Edge>> outgoing_edges = cfg.getOutgoingEdges(leader);
                        assert(outgoing_edges.size() == 1);
                        edges.push_back(std::make_shared<Edge>(outgoing_edges.at(0)->getSourceLabel(),
                                                               outgoing_edges.at(0)->getTargetLabel(),
                                                               outgoing_edges.at(0)->getType()));
                    } else {
                        std::vector<std::unique_ptr<ir::Instruction>> instructions;
                        instructions.reserve(labels.size());
                        for (unsigned int l : labels) {
                            instructions.push_back(cfg.getVertex(l).getInstruction()->clone());
                        }
                        std::unique_ptr<ir::SequenceInstruction> sequence_instruction =
                                std::make_unique<ir::SequenceInstruction>(std::move(instructions));
                        label_to_vertex.emplace(leader,
                                                std::make_shared<Vertex>(leader, std::move(sequence_instruction)));
                        for (const std::shared_ptr<Edge> &outgoing_edge : cfg.getOutgoingEdges(labels.back())) {
                            edges.push_back(std::make_shared<Edge>(leader, outgoing_edge->getTargetLabel(),
                                                                   outgoing_edge->getType()));
                        }
                        vertex = &cfg.getVertex(labels.back());
                        instruction = vertex->getInstruction();
                        if (instruction->getKind() == ir::Instruction::Kind::CALL) {
                            const Edge &intraprocedural_call_to_return_edge =
                                    cfg.getIntraproceduralCallToReturnEdge(labels.back());
                            const Edge &interprocedural_return_edge = cfg.getInterproceduralReturnEdge(
                                    intraprocedural_call_to_return_edge.getTargetLabel());
                            edges.push_back(std::make_shared<Edge>(interprocedural_return_edge.getSourceLabel(),
                                                                   interprocedural_return_edge.getTargetLabel(),
                                                                   interprocedural_return_edge.getCallerName(), leader,
                                                                   Edge::Type::INTERPROCEDURAL_RETURN));
                        } else if (instruction->getKind() == ir::Instruction::Kind::ASSIGNMENT) {
                            // do nothing, an assignment can be the last instruction of a sequence if the succeeding
                            // vertex holds a while instruction
                        } else if (instruction->getKind() == ir::Instruction::Kind::HAVOC) {
                            // do nothing
                        } else if (instruction->getKind() == ir::Instruction::Kind::IF) {
                            // do nothing
                        } else {
                            throw std::logic_error("Not implemented yet.");
                        }
                    }
                    break;
                }
                case ir::Instruction::Kind::CALL: {
                    label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, instruction->clone()));
                    const Edge &interprocedural_call_edge = cfg.getInterproceduralCallEdge(leader);
                    edges.push_back(std::make_shared<Edge>(interprocedural_call_edge.getSourceLabel(),
                                                           interprocedural_call_edge.getTargetLabel(),
                                                           interprocedural_call_edge.getType()));
                    const Edge &intraprocedural_edge = cfg.getIntraproceduralCallToReturnEdge(leader);
                    edges.push_back(std::make_shared<Edge>(intraprocedural_edge.getSourceLabel(),
                                                           intraprocedural_edge.getTargetLabel(),
                                                           intraprocedural_edge.getType()));
                    const Edge &interprocedural_return_edge =
                            cfg.getInterproceduralReturnEdge(intraprocedural_edge.getTargetLabel());
                    edges.push_back(std::make_shared<Edge>(
                            interprocedural_return_edge.getSourceLabel(), interprocedural_return_edge.getTargetLabel(),
                            interprocedural_return_edge.getCallerName(), interprocedural_return_edge.getCallLabel(),
                            Edge::Type::INTERPROCEDURAL_RETURN));
                    break;
                }
                case ir::Instruction::Kind::IF: {
                    label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, instruction->clone()));
                    for (const std::shared_ptr<Edge> &outgoing_edge : cfg.getOutgoingEdges(leader)) {
                        edges.push_back(std::make_shared<Edge>(outgoing_edge->getSourceLabel(),
                                                               outgoing_edge->getTargetLabel(),
                                                               outgoing_edge->getType()));
                    }
                    break;
                }
                case ir::Instruction::Kind::WHILE: {
                    label_to_vertex.emplace(leader, std::make_shared<Vertex>(leader, instruction->clone()));
                    for (const std::shared_ptr<Edge> &outgoing_edge : cfg.getOutgoingEdges(leader)) {
                        edges.push_back(std::make_shared<Edge>(outgoing_edge->getSourceLabel(),
                                                               outgoing_edge->getTargetLabel(),
                                                               outgoing_edge->getType()));
                    }
                    break;
                }
                case ir::Instruction::Kind::GOTO:
                case ir::Instruction::Kind::SEQUENCE:
                    throw std::logic_error("Not implemented yet.");
                default:
                    throw std::runtime_error("Invalid instruction kind.");
            }
        }
    }
    return std::make_shared<Cfg>(cfg.getType(), cfg.getName(), std::move(interface),
                                 std::move(type_representative_name_to_cfg), std::move(label_to_vertex),
                                 std::move(edges), cfg.getEntryLabel(), cfg.getExitLabel());
}