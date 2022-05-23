#include "pass/ssa/ssa_pass.h"
#include "ir/expression/constant/nondeterministic_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/phi.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"

#include <list>

using namespace pass;

const std::string SsaPass::CONSTANT_VARIABLE_NAME_PREFIX = "const";

SsaPass::SsaPass() : _ssa_encoder(std::make_unique<SsaEncoder>(*this)), _cfg(nullptr), _value(0) {}

std::shared_ptr<Cfg> SsaPass::apply(const Cfg &cfg) {
    std::map<std::string, std::shared_ptr<Cfg>> type_representative_name_to_cfg;
    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        std::string type_representative_name = callee_it->getName();
        auto it = _type_representative_name_to_cfg.find(type_representative_name);
        if (it == _type_representative_name_to_cfg.end()) {
            std::shared_ptr<Cfg> ssa_callee_cfg = apply(*callee_it);
            std::cout << ssa_callee_cfg->toDot() << std::endl;
            _value_to_variable.clear();
            _label_to_instructions.clear();
            _filled_basic_blocks.clear();
            _sealed_basic_blocks.clear();
            _current_definitions.clear();
            _value_to_phi_candidate.clear();
            _type_representative_name_to_cfg.emplace(type_representative_name, ssa_callee_cfg);
            type_representative_name_to_cfg.emplace(type_representative_name, ssa_callee_cfg);
        } else {
            type_representative_name_to_cfg.emplace(type_representative_name, it->second);
        }
    }

    _cfg = &cfg;
    for (auto it = cfg.getInterface().variablesBegin(); it != cfg.getInterface().variablesEnd(); ++it) {
        if (it->getStorageType() == ir::Variable::StorageType::TEMPORARY) {
            continue;
        } else {
            int value = _value++;
            std::string name = it->getName();
            if (name.find("_input") != std::string::npos || name.find("_output") != std::string::npos) {
                std::shared_ptr<ir::Variable> initial_ssa_variable = std::make_shared<ir::Variable>(
                        name, it->getDataType().clone(), it->getStorageType(),
                        it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                _value_to_variable.emplace(value, std::move(initial_ssa_variable));
                writeVariable(name, cfg.getEntryLabel(), value);
            } else {
                std::string ssa_name = name + "_" + std::to_string(value);
                std::shared_ptr<ir::Variable> initial_ssa_variable = nullptr;
                // XXX keep input/output relation for main cfg intact, but callees have explicit input/output relations
                // XXX for, e.g., top-down interprocedural analysis of CRAB
                if (cfg.getType() == Cfg::Type::PROGRAM) {
                    initial_ssa_variable = std::make_shared<ir::Variable>(
                            std::move(ssa_name), it->getDataType().clone(), it->getStorageType(),
                            it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                } else {
                    if (it->getStorageType() == ir::Variable::StorageType::INPUT ||
                        it->getStorageType() == ir::Variable::StorageType::OUTPUT) {
                        initial_ssa_variable = std::make_shared<ir::Variable>(
                                std::move(ssa_name), it->getDataType().clone(), ir::Variable::StorageType::LOCAL,
                                it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                    } else {
                        initial_ssa_variable = std::make_shared<ir::Variable>(
                                std::move(ssa_name), it->getDataType().clone(), it->getStorageType(),
                                it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                    }
                }
                assert(initial_ssa_variable != nullptr);
                _value_to_variable.emplace(value, std::move(initial_ssa_variable));
                writeVariable(name, cfg.getEntryLabel(), value);
            }
        }
    }

    // entry of cfg is always sealed (no predecessors) and filled (no instructions)
    _sealed_basic_blocks.insert(_cfg->getEntryLabel());
    {
        const Vertex &vertex = _cfg->getVertex(_cfg->getEntryLabel());
        const ir::Instruction *instruction = vertex.getInstruction();
        if (instruction == nullptr) {
            _filled_basic_blocks.insert(_cfg->getEntryLabel());
        }
    }
    {
        const Vertex &vertex = _cfg->getVertex(_cfg->getExitLabel());
        const ir::Instruction *instruction = vertex.getInstruction();
        if (instruction == nullptr) {
            _filled_basic_blocks.insert(_cfg->getExitLabel());
        }
    }

    for (auto edge_it = cfg.edgesBegin(); edge_it != cfg.edgesEnd(); ++edge_it) {
        if (edge_it->getType() == Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN) {
            _sealed_basic_blocks.insert(edge_it->getTargetLabel());
        }
    }

    std::list<unsigned int> q{_cfg->getEntryLabel()};
    while (!q.empty()) {
        _label = q.front();
        q.pop_front();
        if (_label == _cfg->getEntryLabel()) {
            const Vertex &vertex = _cfg->getVertex(_label);
            const ir::Instruction *instruction = vertex.getInstruction();
            if (instruction != nullptr) {
                instruction->accept(*this);
            }
            _filled_basic_blocks.insert(_cfg->getEntryLabel());
            std::vector<unsigned int> succeeding_labels = _cfg->getSucceedingLabels(_label);
            assert(succeeding_labels.size() == 1);
            q.push_back(succeeding_labels.at(0));
            continue;
        }
        if (isSealable(_label)) {
            sealBasicBlock(_label);
        }
        if (_label == _cfg->getExitLabel() && _sealed_basic_blocks.find(_label) == _sealed_basic_blocks.end()) {
            const Vertex &vertex = _cfg->getVertex(_label);
            const ir::Instruction *instruction = vertex.getInstruction();
            if (instruction != nullptr) {
                instruction->accept(*this);
            }
            _filled_basic_blocks.insert(_cfg->getExitLabel());
            continue;
        }
        if (_sealed_basic_blocks.find(_label) == _sealed_basic_blocks.end()) {
            // not sealed yet, push back to q and process predecessors
            q.push_back(_label);
        } else {
            if (_filled_basic_blocks.find(_label) == _filled_basic_blocks.end()) {
                const Vertex &vertex = _cfg->getVertex(_label);
                const ir::Instruction *instruction = vertex.getInstruction();
                assert(instruction != nullptr);
                instruction->accept(*this);
                _filled_basic_blocks.insert(_label);
                for (const std::shared_ptr<Edge> &outgoing_edge : _cfg->getOutgoingEdges(_label)) {
                    if (outgoing_edge->getType() == Edge::Type::INTERPROCEDURAL_CALL ||
                        outgoing_edge->getType() == Edge::Type::INTERPROCEDURAL_RETURN) {
                        // XXX skip
                        continue;
                    }
                    q.push_back(outgoing_edge->getTargetLabel());
                }
            } else {
                // XXX already processed
                continue;
            }
        }
    }

    // XXX force read of all variables local to this cfg at exit to trigger phi generation
    for (auto it = _cfg->getInterface().variablesBegin(); it != _cfg->getInterface().variablesEnd(); ++it) {
        if (it->getStorageType() == ir::Variable::StorageType::TEMPORARY ||
            it->getDataType().getKind() == ir::DataType::Kind::DERIVED_TYPE) {
            continue;
        }
        const std::string &name = it->getName();
        readVariable(name, _cfg->getExitLabel());
    }

    for (const auto &value_to_phi_candidate : _value_to_phi_candidate) {
        int value = value_to_phi_candidate.first;
        std::shared_ptr<PhiCandidate> phi_candidate = value_to_phi_candidate.second;
        unsigned int label = phi_candidate->getLabel();

        std::map<unsigned int, std::unique_ptr<ir::VariableAccess>> label_to_operand;
        for (int value_of_operand : phi_candidate->getValuesOfOperands()) {
            std::shared_ptr<ir::Variable> variable = _value_to_variable.at(value_of_operand);
            std::string name = variable->getName();
            std::size_t position = name.find('_');
            assert(position != std::string::npos);
            for (const auto &label_to_value : _current_definitions.at(name.substr(0, position))) {
                if (label_to_value.second == value_of_operand) {
                    auto it = std::find_if(label_to_operand.begin(), label_to_operand.end(),
                                           [name](const auto &label_to_operand) {
                                               return label_to_operand.second->getName() == name;
                                           });
                    if (it == label_to_operand.end()) {
                        label_to_operand.emplace(label_to_value.first, std::make_unique<ir::VariableAccess>(variable));
                    } else {
                        if (it->first > label_to_value.first) {
                            label_to_operand.erase(it);
                            label_to_operand.emplace(label_to_value.first,
                                                     std::make_unique<ir::VariableAccess>(variable));
                        }
                    }
                    continue;
                }
            }
        }
        std::unique_ptr<ir::Phi> phi_expression =
                std::make_unique<ir::Phi>(_value_to_variable.at(value), std::move(label_to_operand));

        std::unique_ptr<ir::AssignmentInstruction> phi_assignment_instruction =
                std::make_unique<ir::AssignmentInstruction>(
                        std::make_unique<ir::VariableAccess>(_value_to_variable.at(value)), std::move(phi_expression));
        insertInstructionAtLabel(std::move(phi_assignment_instruction), label, true);
    }

    std::vector<std::shared_ptr<ir::Variable>> variables;
    for (auto &value_to_variable : _value_to_variable) {
        std::shared_ptr<ir::Variable> variable = value_to_variable.second;
        std::string name = variable->getName();
        if (cfg.getType() != Cfg::Type::PROGRAM) {
            if (name.find("_input") == std::string::npos && name.find("_output") == std::string::npos) {
                if (variable->getStorageType() == ir::Variable::StorageType::INPUT ||
                    variable->getStorageType() == ir::Variable::StorageType::OUTPUT) {

                    variable->setStorageType(ir::Variable::StorageType::LOCAL);
                }
            }
            variables.push_back(variable);
        } else {
            variables.push_back(variable);
        }
    }

    // build basic blocks and edge relation
    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        unsigned int label = it->getLabel();
        if (label == cfg.getEntryLabel() || label == cfg.getExitLabel()) {
            if (_label_to_instructions.find(label) != _label_to_instructions.end()) {
                std::vector<std::unique_ptr<ir::Instruction>> &instructions = _label_to_instructions.at(label);
                std::shared_ptr<Vertex> vertex = std::make_shared<Vertex>(label, it->getType());
                if (instructions.size() > 1) {
                    std::unique_ptr<ir::SequenceInstruction> sequence_instruction =
                            std::make_unique<ir::SequenceInstruction>(std::move(instructions));
                    vertex->setInstruction(std::move(sequence_instruction));
                } else {
                    vertex->setInstruction(std::move(instructions.at(0)));
                }
                label_to_vertex.emplace(label, std::move(vertex));
            } else {
                label_to_vertex.emplace(label, std::make_shared<Vertex>(label, it->getType()));
            }
        } else {
            std::vector<std::unique_ptr<ir::Instruction>> &instructions = _label_to_instructions.at(label);
            std::shared_ptr<Vertex> vertex = std::make_shared<Vertex>(label, it->getType());
            if (instructions.size() > 1) {
                std::unique_ptr<ir::SequenceInstruction> sequence_instruction =
                        std::make_unique<ir::SequenceInstruction>(std::move(instructions));
                vertex->setInstruction(std::move(sequence_instruction));
            } else {
                vertex->setInstruction(std::move(instructions.at(0)));
            }
            label_to_vertex.emplace(label, std::move(vertex));
        }
    }

    std::vector<std::shared_ptr<Edge>> edges;
    for (auto it = cfg.edgesBegin(); it != cfg.edgesEnd(); ++it) {
        switch (it->getType()) {
            case Edge::Type::INTRAPROCEDURAL: {
                // XXX fall-through
            }
            case Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN: {
                // XXX fall-through
            }
            case Edge::Type::INTERPROCEDURAL_CALL: {
                // XXX fall-through
            }
            case Edge::Type::TRUE_BRANCH: {
                // XXX fall-through
            }
            case Edge::Type::FALSE_BRANCH: {
                edges.push_back(std::make_shared<Edge>(it->getSourceLabel(), it->getTargetLabel(), it->getType()));
                break;
            }
            case Edge::Type::INTERPROCEDURAL_RETURN: {
                edges.push_back(std::make_shared<Edge>(it->getSourceLabel(), it->getTargetLabel(), it->getCallerName(),
                                                       it->getCallLabel(), it->getType()));
                break;
            }
            default:
                throw std::runtime_error("Unexpected edge type encountered.");
        }
    }

    std::unique_ptr<ir::Interface> interface = std::make_unique<ir::Interface>(std::move(variables));

    for (auto it = interface->variablesBegin(); it != interface->variablesEnd(); ++it) {
        std::cout << it->getName() << ": " << it->getStorageType() << std::endl;
    }

    std::shared_ptr<Cfg> ssa_cfg = std::make_shared<Cfg>(
            cfg.getType(), cfg.getName(), std::move(interface), std::move(type_representative_name_to_cfg),
            std::move(label_to_vertex), std::move(edges), cfg.getEntryLabel(), cfg.getExitLabel());

    return ssa_cfg;
}

void SsaPass::insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label,
                                       bool in_front) {
    if (_label_to_instructions.find(label) == _label_to_instructions.end()) {
        std::vector<std::unique_ptr<ir::Instruction>> instructions;
        instructions.push_back(std::move(instruction));
        _label_to_instructions.emplace(label, std::move(instructions));
    } else {
        if (in_front) {
            _label_to_instructions.at(label).insert(_label_to_instructions.at(label).begin(), std::move(instruction));
        } else {
            _label_to_instructions.at(label).push_back(std::move(instruction));
        }
    }
}

void SsaPass::writeVariable(const std::string &name, unsigned int label, int value) {
    _current_definitions[name][label] = value;
    auto it = _value_to_phi_candidate.find(value);
    if (it != _value_to_phi_candidate.end()) {
        std::cout << "phi user " << label << " in write variable" << std::endl;
    }
}

int SsaPass::readVariable(const std::string &name, unsigned int label) {
    if (_current_definitions.find(name) != _current_definitions.end()) {
        auto it = _current_definitions.at(name).find(label);
        if (it != _current_definitions.at(name).end()) {
            // local value numbering, return the definition of the variable at this particular block
            return it->second;
        }
    } else {
        throw std::runtime_error("No definition for variable exists.");
    }
    // global value numbering, look in preceding blocks for the value of the variable
    return readVariableRecursive(name, label);
}

int SsaPass::readVariableRecursive(const std::string &name, unsigned int label) {
    int value = -1;
    auto it = _sealed_basic_blocks.find(label);
    if (it == _sealed_basic_blocks.end()) {
        // incomplete cfg
        throw std::logic_error("Not implemented yet.");
    } else {
        std::vector<unsigned int> preceding_labels;
        for (const auto &incoming_edge : _cfg->getIncomingEdges(label)) {
            if (incoming_edge->getType() == Edge::Type::INTERPROCEDURAL_RETURN) {
                // XXX skip
                continue;
            }
            preceding_labels.push_back(incoming_edge->getSourceLabel());
        }
        if (preceding_labels.size() == 1) {
            value = readVariable(name, preceding_labels.at(0));
        } else {
            value = placeOperandlessPhi(name, label);
            writeVariable(name, label, value);
            value = addPhiOperands(name, value);
        }
    }
    if (value == -1) {
        throw std::logic_error("Not implemented yet.");
    } else {
        writeVariable(name, label, value);
    }
    return value;
}

bool SsaPass::isSealable(unsigned int label) const {
    bool sealable = true;
    // a block is sealable if it is connected to all its direct control-flow predecessors and all direct control-flow
    // predecessors are filled
    for (unsigned int preceding_label : _cfg->getPrecedingLabels(label)) {
        if (_filled_basic_blocks.find(preceding_label) == _filled_basic_blocks.end()) {
            sealable = false;
            break;
        }
    }
    return sealable;
}

void SsaPass::sealBasicBlock(unsigned int label) {
    _sealed_basic_blocks.insert(label);
}

int SsaPass::placeOperandlessPhi(const std::string &name, unsigned int label) {
    int value = _value++;
    _value_to_phi_candidate.emplace(value, std::make_shared<PhiCandidate>(name, label, value));
    // TODO (26.02.2022): Hacky, could not be in the same scope!
    auto it = std::find_if(_cfg->getInterface().variablesBegin(), _cfg->getInterface().variablesEnd(),
                           [name](const auto &variable) { return variable.getName() == name; });
    if (it == _cfg->getInterface().variablesEnd()) {
        throw std::runtime_error("Variable does not exist in this cfg.");
    }
    _value_to_variable.emplace(value, std::make_shared<ir::Variable>(name + "_" + std::to_string(value),
                                                                     it->getDataType().clone(), it->getStorageType()));
    return value;
}

int SsaPass::addPhiOperands(const std::string &name, int value) {
    auto it = _value_to_phi_candidate.find(value);
    if (it == _value_to_phi_candidate.end()) {
        throw std::runtime_error("Expected phi candidate does not exist.");
    }
    std::shared_ptr<PhiCandidate> phi_candidate = it->second;
    std::vector<unsigned int> preceding_labels = _cfg->getPrecedingLabels(phi_candidate->getLabel());
    for (unsigned int preceding_label : preceding_labels) {
        int operand_value = readVariable(name, preceding_label);
        phi_candidate->appendValuesOfOperands(operand_value);
        it = _value_to_phi_candidate.find(operand_value);
        if (it != _value_to_phi_candidate.end()) {
            if (*phi_candidate != *it->second) {
                it->second->addValuesOfUsers(phi_candidate->getValue());
            }
        }
    }
    return tryRemoveTrivialPhi(*phi_candidate);
}

int SsaPass::tryRemoveTrivialPhi(PhiCandidate &phi_candidate) {
    int value = phi_candidate.getValue();
    int same = -1;
    for (int value_of_operand : phi_candidate.getValuesOfOperands()) {
        if (value_of_operand == same || value_of_operand == value) {
            continue;
        }
        if (same != -1) {
            return value;
        }
        same = value_of_operand;
    }
    if (same == -1) {
        throw std::logic_error("Not implemented yet.");
    }
    std::vector<int> values_of_users = phi_candidate.getValuesOfUsers();
    auto it = std::find(values_of_users.begin(), values_of_users.end(), value);
    if (it != values_of_users.end()) {
        values_of_users.erase(it);
    }
    // Reroute all uses of phi to same and remove phi
    for (int value_of_user : values_of_users) {
        auto user_it = _value_to_phi_candidate.find(value_of_user);
        if (user_it == _value_to_phi_candidate.end()) {
            // user is NOT a phi, this case is not handled yet!
            throw std::logic_error("Not implemented yet.");
        } else {
            std::shared_ptr<PhiCandidate> using_phi_candidate = user_it->second;
            using_phi_candidate->replaceValueOfUsedPhiCandidate(value, same);
        }
    }
    _value_to_phi_candidate.erase(value);
    _value_to_variable.erase(value);
    for (int value_of_user : values_of_users) {
        auto phi_candidate_it = _value_to_phi_candidate.find(value_of_user);
        if (phi_candidate_it != _value_to_phi_candidate.end()) {
            tryRemoveTrivialPhi(*phi_candidate_it->second);
        }
    }
    return same;
}

void SsaPass::visit(const ir::AssignmentInstruction &instruction) {
    switch (instruction.getType()) {
        case ir::AssignmentInstruction::Type::REGULAR: {
            const ir::Expression &expression = instruction.getExpression();
            // encode rhs of the assignment
            std::unique_ptr<ir::Expression> encoded_expression = _ssa_encoder->encode(expression);

            // encode lhs of the assignment
            int value = _value++;
            const ir::VariableReference &variable_reference = instruction.getVariableReference();
            std::string name = variable_reference.getName();
            std::string fresh_name = name;
            // XXX distinguish between explicit output variables due to call transformation
            if (name.find("_output") == std::string::npos) {
                fresh_name = fresh_name + "_" + std::to_string(value);
            }

            std::unique_ptr<ir::AssignmentInstruction> assignment_instruction = nullptr;
            if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
                std::shared_ptr<ir::Variable> variable = variable_access->getVariable();
                std::shared_ptr<ir::Variable> fresh_variable = std::make_shared<ir::Variable>(
                        fresh_name, variable->getDataType().clone(), variable->getStorageType());

                _value_to_variable.emplace(value, fresh_variable);

                // introduce fresh variable expression for lhs
                std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                        std::make_unique<ir::VariableAccess>(fresh_variable);

                assignment_instruction = std::make_unique<ir::AssignmentInstruction>(std::move(fresh_variable_access),
                                                                                     std::move(encoded_expression));
            } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
                throw std::logic_error("Not implemented yet.");
            } else {
                throw std::runtime_error("Unexpected variable reference type encountered.");
            }
            assert(assignment_instruction != nullptr);
            insertInstructionAtLabel(std::move(assignment_instruction), _label);
            writeVariable(name, _label, value);
            break;
        }
        case ir::AssignmentInstruction::Type::PARAMETER_IN: {
            const ir::Expression &expression = instruction.getExpression();
            // encode rhs of the assignment
            std::unique_ptr<ir::Expression> encoded_expression = _ssa_encoder->encode(expression);

            // encode lhs of the assignment
            auto *field_access = dynamic_cast<const ir::FieldAccess *>(&instruction.getVariableReference());
            assert(field_access != nullptr);
            const ir::VariableAccess record_access = field_access->getRecordAccess();
            std::shared_ptr<ir::Variable> callee_variable = record_access.getVariable();
            std::string type_representative_name = callee_variable->getDataType().getName();
            std::shared_ptr<Cfg> callee_cfg = _type_representative_name_to_cfg.at(type_representative_name);
            const ir::VariableAccess variable_access = field_access->getVariableAccess();
            std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
            std::string variable_name = variable->getName();

            // introduce fresh variable expression for lhs
            std::shared_ptr<ir::Variable> fresh_variable = std::make_shared<ir::Variable>(
                    variable_name, variable->getDataType().clone(), variable->getStorageType());
            std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                    std::make_unique<ir::VariableAccess>(fresh_variable);
            std::unique_ptr<ir::FieldAccess> fresh_field_access =
                    std::make_unique<ir::FieldAccess>(record_access.clone(), std::move(fresh_variable_access));

            std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
                    std::make_unique<ir::AssignmentInstruction>(std::move(fresh_field_access),
                                                                std::move(encoded_expression),
                                                                ir::AssignmentInstruction::Type::PARAMETER_IN);

            insertInstructionAtLabel(std::move(assignment_instruction), _label);
            break;
        }
        case ir::AssignmentInstruction::Type::PARAMETER_OUT: {
            // encode rhs of the assignment
            // TODO: (01.03.2022): It doesn't have to be a field access, it can also be a constant.
            std::unique_ptr<ir::Expression> encoded_expression = nullptr;
            if (auto *field_access = dynamic_cast<const ir::FieldAccess *>(&instruction.getExpression())) {
                const ir::VariableAccess record_access = field_access->getRecordAccess();
                std::shared_ptr<ir::Variable> callee_variable = record_access.getVariable();
                std::string type_representative_name = callee_variable->getDataType().getName();
                std::shared_ptr<Cfg> callee_cfg = _type_representative_name_to_cfg.at(type_representative_name);
                const ir::VariableAccess variable_access = field_access->getVariableAccess();
                std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
                std::string variable_name = variable->getName();
                const ir::Interface &interface = callee_cfg->getInterface();

                // introduce fresh variable expression for rhs
                std::shared_ptr<ir::Variable> fresh_variable = std::make_shared<ir::Variable>(
                        variable_name, variable->getDataType().clone(), variable->getStorageType());
                std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                        std::make_unique<ir::VariableAccess>(fresh_variable);
                encoded_expression =
                        std::make_unique<ir::FieldAccess>(record_access.clone(), std::move(fresh_variable_access));
            } else {
                throw std::logic_error("Not implemented yet.");
            }
            assert(encoded_expression != nullptr);

            // encode lhs of the assignment
            int value = _value++;
            const ir::VariableReference &variable_reference = instruction.getVariableReference();
            std::string name = variable_reference.getName();
            std::string fresh_name = name + "_" + std::to_string(value);

            std::unique_ptr<ir::AssignmentInstruction> assignment_instruction = nullptr;
            if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
                std::shared_ptr<ir::Variable> variable = variable_access->getVariable();
                std::shared_ptr<ir::Variable> fresh_variable = std::make_shared<ir::Variable>(
                        fresh_name, variable->getDataType().clone(), variable->getStorageType());
                _value_to_variable.emplace(value, fresh_variable);

                // introduce fresh variable expression for lhs
                std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                        std::make_unique<ir::VariableAccess>(fresh_variable);

                assignment_instruction = std::make_unique<ir::AssignmentInstruction>(
                        std::move(fresh_variable_access), std::move(encoded_expression),
                        ir::AssignmentInstruction::Type::PARAMETER_OUT);
            } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
                const ir::VariableAccess &record_access = field_access->getRecordAccess();
                std::shared_ptr<ir::Variable> variable = field_access->getVariableAccess().getVariable();
                std::shared_ptr<ir::Variable> fresh_variable =
                        std::make_shared<ir::Variable>(variable->getName() + "_" + std::to_string(value),
                                                       variable->getDataType().clone(), variable->getStorageType());
                _value_to_variable.emplace(value, fresh_variable);

                // introduce fresh variable expression for lhs
                std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                        std::make_unique<ir::VariableAccess>(fresh_variable);

                std::unique_ptr<ir::FieldAccess> fresh_field_access =
                        std::make_unique<ir::FieldAccess>(record_access.clone(), std::move(fresh_variable_access));

                assignment_instruction = std::make_unique<ir::AssignmentInstruction>(
                        std::move(fresh_field_access), std::move(encoded_expression),
                        ir::AssignmentInstruction::Type::PARAMETER_OUT);
            } else {
                throw std::runtime_error("Unexpected variable reference type encountered.");
            }
            assert(assignment_instruction != nullptr);
            insertInstructionAtLabel(std::move(assignment_instruction), _label);
            writeVariable(name, _label, value);
            break;
        }
        default:
            throw std::runtime_error("Unexpected instruction type encountered.");
    }
}

void SsaPass::visit(const ir::CallInstruction &instruction) {
    // XXX the intermediate representation follows the call-by-value paradigm using a fully flattened interface, where
    // XXX arguments and assignments have been lowered to pre- and post-assignments during compilation. The explicit
    // XXX handling of call instructions can therefore be skipped, however, parameter in- and out-assignments have to
    // XXX be considered in the respective visitor of the assignment instruction.
    insertInstructionAtLabel(instruction.clone(), _label);
}

void SsaPass::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();
    // encode the condition
    std::unique_ptr<ir::Expression> encoded_expression = _ssa_encoder->encode(expression);

    std::unique_ptr<ir::IfInstruction> if_instruction = std::make_unique<ir::IfInstruction>(
            std::move(encoded_expression), instruction.getGotoThen().clone(), instruction.getGotoElse().clone());
    insertInstructionAtLabel(std::move(if_instruction), _label);
}

void SsaPass::visit(const ir::SequenceInstruction &instruction) {
    for (auto it = instruction.instructionsBegin(); it != instruction.instructionsEnd(); ++it) {
        it->accept(*this);
    }
}

void SsaPass::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void SsaPass::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void SsaPass::visit(const ir::HavocInstruction &instruction) {
    // encode lhs of the assignment
    int value = _value++;
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string fresh_name = name;
    // XXX distinguish between explicit output variables due to call transformation
    if (name.find("_output") == std::string::npos) {
        fresh_name = fresh_name + "_" + std::to_string(value);
    }

    std::unique_ptr<ir::HavocInstruction> havoc_instruction = nullptr;
    if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
        std::shared_ptr<ir::Variable> variable = variable_access->getVariable();
        std::shared_ptr<ir::Variable> fresh_variable =
                std::make_shared<ir::Variable>(fresh_name, variable->getDataType().clone(), variable->getStorageType());

        _value_to_variable.emplace(value, fresh_variable);

        // introduce fresh variable expression for lhs
        std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                std::make_unique<ir::VariableAccess>(fresh_variable);

        havoc_instruction = std::make_unique<ir::HavocInstruction>(std::move(fresh_variable_access));
    } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
        throw std::logic_error("Not implemented yet.");
    } else {
        throw std::runtime_error("Unexpected variable reference type encountered.");
    }
    assert(havoc_instruction != nullptr);
    insertInstructionAtLabel(std::move(havoc_instruction), _label);
    writeVariable(name, _label, value);
}
