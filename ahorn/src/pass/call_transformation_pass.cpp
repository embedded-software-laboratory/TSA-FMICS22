#include "pass/call_transformation_pass.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"

#include <map>
#include <memory>

using namespace pass;

std::shared_ptr<Cfg> CallTransformationPass::apply(const Cfg &cfg) {
    std::map<std::string, std::shared_ptr<Cfg>> type_representative_name_to_cfg;
    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        std::string type_representative_name = callee_it->getName();
        auto it = _type_representative_name_to_cfg.find(type_representative_name);
        if (it == _type_representative_name_to_cfg.end()) {
            std::shared_ptr<Cfg> callee_cfg = apply(*callee_it);
            _label_to_instructions.clear();
            _type_representative_name_to_cfg.emplace(type_representative_name, callee_cfg);
            type_representative_name_to_cfg.emplace(type_representative_name, callee_cfg);
        } else {
            type_representative_name_to_cfg.emplace(type_representative_name, it->second);
        }
    }

    _cfg = &cfg;
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        _label = it->getLabel();
        if (_label != cfg.getEntryLabel() && _label != cfg.getExitLabel()) {
            ir::Instruction *instruction = it->getInstruction();
            assert(instruction != nullptr);
            instruction->accept(*this);
        }
    }

    std::vector<std::shared_ptr<ir::Variable>> variables;
    std::vector<std::pair<std::shared_ptr<ir::Variable>, std::shared_ptr<ir::Variable>>> augmented_input_variables;
    std::vector<std::pair<std::shared_ptr<ir::Variable>, std::shared_ptr<ir::Variable>>> augmented_output_variables;
    std::map<std::string, std::vector<std::pair<std::string, std::shared_ptr<ir::Variable>>>> augmented_state_variables;
    for (auto it = cfg.getInterface().variablesBegin(); it != cfg.getInterface().variablesEnd(); ++it) {
        const std::string &name = it->getName();
        const ir::DataType &data_type = it->getDataType();
        const ir::Variable::StorageType storage_type = it->getStorageType();
        switch (data_type.getKind()) {
            case ir::DataType::Kind::SAFETY_TYPE: {
                // XXX fall-through
            }
            case ir::DataType::Kind::ELEMENTARY_TYPE: {
                std::shared_ptr<ir::Variable> variable = std::make_shared<ir::Variable>(
                        name, data_type.clone(), it->getStorageType(),
                        it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                variables.push_back(variable);
                switch (storage_type) {
                    case ir::Variable::StorageType::INPUT: {
                        if (cfg.getType() != Cfg::Type::PROGRAM) {
                            std::shared_ptr<ir::Variable> augmented_variable = std::make_shared<ir::Variable>(
                                    name + "_input", data_type.clone(), ir::Variable::StorageType::INPUT,
                                    it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                            variables.push_back(augmented_variable);
                            augmented_input_variables.emplace_back(variable, augmented_variable);
                        }
                        break;
                    }
                    case ir::Variable::StorageType::LOCAL:
                        if (cfg.getType() != Cfg::Type::PROGRAM) {
                            std::shared_ptr<ir::Variable> augmented_input_variable = std::make_shared<ir::Variable>(
                                    name + "_input", data_type.clone(), ir::Variable::StorageType::INPUT,
                                    it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                            variables.push_back(augmented_input_variable);
                            augmented_input_variables.emplace_back(variable, augmented_input_variable);
                            std::shared_ptr<ir::Variable> augmented_output_variable = std::make_shared<ir::Variable>(
                                    name + "_output", data_type.clone(), ir::Variable::StorageType::OUTPUT,
                                    it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                            variables.push_back(augmented_output_variable);
                            augmented_output_variables.emplace_back(augmented_output_variable, variable);
                        }
                        break;
                    case ir::Variable::StorageType::OUTPUT:
                        if (cfg.getType() != Cfg::Type::PROGRAM) {
                            std::shared_ptr<ir::Variable> augmented_variable = std::make_shared<ir::Variable>(
                                    name + "_output", data_type.clone(), ir::Variable::StorageType::OUTPUT,
                                    it->hasInitialization() ? it->getInitialization().clone() : nullptr);
                            variables.push_back(augmented_variable);
                            augmented_output_variables.emplace_back(augmented_variable, variable);
                        }
                        break;
                    case ir::Variable::StorageType::TEMPORARY:
                        break;
                    default:
                        throw std::runtime_error("Unexpected storage type encountered.");
                }
                break;
            }
            case ir::DataType::Kind::DERIVED_TYPE: {
                assert(!it->hasInitialization());
                augmented_state_variables.emplace(name,
                                                  std::vector<std::pair<std::string, std::shared_ptr<ir::Variable>>>());
                variables.push_back(std::make_shared<ir::Variable>(name, data_type.clone(), it->getStorageType()));
                const std::string &type_representative_name = data_type.getName();
                const std::shared_ptr<Cfg> &type_representative_cfg =
                        type_representative_name_to_cfg.at(type_representative_name);
                // XXX augment cfg with state variables of callee
                for (auto nested_it = type_representative_cfg->getInterface().localVariablesBegin();
                     nested_it != type_representative_cfg->getInterface().localVariablesEnd(); ++nested_it) {
                    const std::string &nested_name = nested_it->getName();
                    const ir::DataType &nested_data_type = nested_it->getDataType();
                    switch (nested_data_type.getKind()) {
                        case ir::DataType::Kind::ELEMENTARY_TYPE: {
                            std::string flattened_name = name + nested_name;
                            std::shared_ptr<ir::Variable> nested_variable = std::make_shared<ir::Variable>(
                                    flattened_name, nested_data_type.clone(), nested_it->getStorageType(),
                                    nested_it->hasInitialization() ? nested_it->getInitialization().clone() : nullptr);
                            variables.push_back(nested_variable);
                            augmented_state_variables.at(name).push_back(std::pair(nested_name, nested_variable));
                            break;
                        }
                        case ir::DataType::Kind::DERIVED_TYPE: {
                            // XXX deeper nested dependencies are already processed as we recurse on each callee until
                            // XXX we are at the end of the call-chain
                            break;
                        }
                        case ir::DataType::Kind::SAFETY_TYPE:
                        case ir::DataType::Kind::ENUMERATED_TYPE:
                        case ir::DataType::Kind::INCONCLUSIVE_TYPE:
                        case ir::DataType::Kind::SIMPLE_TYPE:
                            throw std::logic_error("Not implemented yet.");
                        default:
                            throw std::runtime_error("Unexpected data type encountered.");
                    }
                }
                break;
            }
            case ir::DataType::Kind::ENUMERATED_TYPE:
            case ir::DataType::Kind::INCONCLUSIVE_TYPE:
            case ir::DataType::Kind::SIMPLE_TYPE:
                throw std::logic_error("Not implemented yet.");
            default:
                throw std::runtime_error("Unexpected data type encountered.");
        }
    }

    // augment call instructions with state assignments
    for (const auto &call_variable_name_to_augmented_state_variables : augmented_state_variables) {
        const std::string &callee_variable_name = call_variable_name_to_augmented_state_variables.first;
        const std::vector<std::pair<std::string, std::shared_ptr<ir::Variable>>> &names_to_state_variables =
                call_variable_name_to_augmented_state_variables.second;
        if (!names_to_state_variables.empty()) {
            for (auto &label_to_instructions : _label_to_instructions) {
                unsigned int label = label_to_instructions.first;
                bool found_call_instruction = false;
                std::shared_ptr<ir::Variable> callee_variable = nullptr;
                for (const auto &instruction : label_to_instructions.second) {
                    if (auto *call_instruction = dynamic_cast<const ir::CallInstruction *>(&*instruction)) {
                        if (callee_variable_name == call_instruction->getVariableAccess().getName()) {
                            callee_variable = call_instruction->getVariableAccess().getVariable();
                            found_call_instruction = true;
                            break;
                        }
                    }
                }
                if (found_call_instruction) {
                    assert(callee_variable != nullptr);
                    // XXX create parameter in assignments
                    for (auto &name_to_state_variable : names_to_state_variables) {
                        const std::string &name = name_to_state_variable.first;
                        std::shared_ptr<ir::Variable> fresh_variable =
                                std::make_shared<ir::Variable>(name + "_input", callee_variable->getDataType().clone(),
                                                               ir::Variable::StorageType::INPUT);
                        std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                                std::make_unique<ir::VariableAccess>(fresh_variable);
                        std::unique_ptr<ir::FieldAccess> fresh_field_access =
                                std::make_unique<ir::FieldAccess>(std::make_unique<ir::VariableAccess>(callee_variable),
                                                                  std::move(fresh_variable_access));

                        std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
                                std::make_unique<ir::AssignmentInstruction>(
                                        std::move(fresh_field_access),
                                        std::make_unique<ir::VariableAccess>(name_to_state_variable.second),
                                        ir::AssignmentInstruction::Type::PARAMETER_IN);
                        insertInstructionAtLabel(std::move(assignment_instruction), label, true);
                    }
                    // XXX create parameter out assignments
                    const Edge &edge = cfg.getIntraproceduralCallToReturnEdge(label);
                    unsigned int next_label = edge.getTargetLabel();
                    for (auto &name_to_state_variable : names_to_state_variables) {
                        const std::string &name = name_to_state_variable.first;
                        // introduce fresh variable expression for rhs
                        std::shared_ptr<ir::Variable> fresh_variable =
                                std::make_shared<ir::Variable>(name + "_output", callee_variable->getDataType().clone(),
                                                               ir::Variable::StorageType::OUTPUT);
                        std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                                std::make_unique<ir::VariableAccess>(fresh_variable);

                        std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
                                std::make_unique<ir::AssignmentInstruction>(
                                        std::make_unique<ir::VariableAccess>(name_to_state_variable.second),
                                        std::make_unique<ir::FieldAccess>(
                                                std::make_unique<ir::VariableAccess>(callee_variable),
                                                std::move(fresh_variable_access)),
                                        ir::AssignmentInstruction::Type::PARAMETER_OUT);
                        insertInstructionAtLabel(std::move(assignment_instruction), next_label, true);
                    }
                }
            }
        }
    }

    // build basic blocks and edge relation
    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        unsigned int label = it->getLabel();
        auto vertex = std::make_shared<Vertex>(label, it->getType());
        if (cfg.getType() == Cfg::Type::PROGRAM) {
            if (label != cfg.getEntryLabel() && label != cfg.getExitLabel()) {
                std::vector<std::unique_ptr<ir::Instruction>> &instructions = _label_to_instructions.at(label);
                if (instructions.size() > 1) {
                    std::unique_ptr<ir::SequenceInstruction> sequence_instruction =
                            std::make_unique<ir::SequenceInstruction>(std::move(instructions));
                    vertex->setInstruction(std::move(sequence_instruction));
                } else {
                    vertex->setInstruction(std::move(instructions.at(0)));
                }
            }
            label_to_vertex.emplace(label, vertex);
        } else {
            // XXX add explicit input/output assignments to the entry and exit, respectively
            if (label == cfg.getEntryLabel()) {
                assert(_label_to_instructions.find(label) == _label_to_instructions.end());
                std::vector<std::unique_ptr<ir::Instruction>> augmented_input_assignments;
                for (auto &augmented_input_variable : augmented_input_variables) {
                    std::unique_ptr<ir::AssignmentInstruction> augmented_input_assignment =
                            std::make_unique<ir::AssignmentInstruction>(
                                    std::make_unique<ir::VariableAccess>(augmented_input_variable.first),
                                    std::make_unique<ir::VariableAccess>(augmented_input_variable.second));
                    augmented_input_assignments.push_back(std::move(augmented_input_assignment));
                }
                std::unique_ptr<ir::SequenceInstruction> augmented_input_assignment =
                        std::make_unique<ir::SequenceInstruction>(std::move(augmented_input_assignments));
                vertex->setInstruction(std::move(augmented_input_assignment));
            } else if (label == cfg.getExitLabel()) {
                assert(_label_to_instructions.find(label) == _label_to_instructions.end());
                std::vector<std::unique_ptr<ir::Instruction>> augmented_output_assignments;
                for (auto &augmented_output_variable : augmented_output_variables) {
                    std::unique_ptr<ir::AssignmentInstruction> augmented_output_assignment =
                            std::make_unique<ir::AssignmentInstruction>(
                                    std::make_unique<ir::VariableAccess>(augmented_output_variable.first),
                                    std::make_unique<ir::VariableAccess>(augmented_output_variable.second));
                    augmented_output_assignments.push_back(std::move(augmented_output_assignment));
                }
                std::unique_ptr<ir::SequenceInstruction> augmented_output_assignment =
                        std::make_unique<ir::SequenceInstruction>(std::move(augmented_output_assignments));
                vertex->setInstruction(std::move(augmented_output_assignment));
            } else {
                std::vector<std::unique_ptr<ir::Instruction>> &instructions = _label_to_instructions.at(label);
                if (instructions.size() > 1) {
                    std::unique_ptr<ir::SequenceInstruction> sequence_instruction =
                            std::make_unique<ir::SequenceInstruction>(std::move(instructions));
                    vertex->setInstruction(std::move(sequence_instruction));
                } else {
                    vertex->setInstruction(std::move(instructions.at(0)));
                }
            }
            label_to_vertex.emplace(label, vertex);
        }
    }

    std::vector<std::shared_ptr<Edge>> edges;
    for (auto it = cfg.edgesBegin(); it != cfg.edgesEnd(); ++it) {
        switch (it->getType()) {
            case Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN: {
                // XXX fall-through
            }
            case Edge::Type::INTRAPROCEDURAL: {
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

    return std::make_shared<Cfg>(cfg.getType(), cfg.getName(), std::move(interface),
                                 std::move(type_representative_name_to_cfg), std::move(label_to_vertex),
                                 std::move(edges), cfg.getEntryLabel(), cfg.getExitLabel());
}

void CallTransformationPass::visit(const ir::AssignmentInstruction &instruction) {
    switch (instruction.getType()) {
        case ir::AssignmentInstruction::Type::REGULAR: {
            insertInstructionAtLabel(instruction.clone(), _label);
            break;
        }
        case ir::AssignmentInstruction::Type::PARAMETER_IN: {
            auto *field_access = dynamic_cast<const ir::FieldAccess *>(&instruction.getVariableReference());
            assert(field_access != nullptr);
            const ir::VariableAccess record_access = field_access->getRecordAccess();
            std::shared_ptr<ir::Variable> callee_variable = record_access.getVariable();
            std::string type_representative_name = callee_variable->getDataType().getName();
            const std::shared_ptr<Cfg> &cfg = _type_representative_name_to_cfg.at(type_representative_name);
            const ir::VariableAccess variable_access = field_access->getVariableAccess();
            std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
            std::string variable_name = variable->getName();
            const ir::Interface &interface = cfg->getInterface();

            // introduce fresh variable expression for lhs
            std::shared_ptr<ir::Variable> fresh_variable = interface.getVariable(variable_name + "_input");
            std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                    std::make_unique<ir::VariableAccess>(fresh_variable);
            std::unique_ptr<ir::FieldAccess> fresh_field_access =
                    std::make_unique<ir::FieldAccess>(record_access.clone(), std::move(fresh_variable_access));

            std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
                    std::make_unique<ir::AssignmentInstruction>(std::move(fresh_field_access),
                                                                instruction.getExpression().clone(),
                                                                ir::AssignmentInstruction::Type::PARAMETER_IN);

            insertInstructionAtLabel(std::move(assignment_instruction), _label);
            break;
        }
        case ir::AssignmentInstruction::Type::PARAMETER_OUT: {
            std::unique_ptr<ir::Expression> encoded_expression = nullptr;
            if (auto *field_access = dynamic_cast<const ir::FieldAccess *>(&instruction.getExpression())) {
                const ir::VariableAccess record_access = field_access->getRecordAccess();
                std::shared_ptr<ir::Variable> callee_variable = record_access.getVariable();
                std::string type_representative_name = callee_variable->getDataType().getName();
                const std::shared_ptr<Cfg> &cfg = _type_representative_name_to_cfg.at(type_representative_name);
                const ir::VariableAccess variable_access = field_access->getVariableAccess();
                std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
                std::string variable_name = variable->getName();
                const ir::Interface &interface = cfg->getInterface();

                // introduce fresh variable expression for rhs
                std::shared_ptr<ir::Variable> fresh_variable = interface.getVariable(variable_name + "_output");
                std::unique_ptr<ir::VariableAccess> fresh_variable_access =
                        std::make_unique<ir::VariableAccess>(fresh_variable);
                encoded_expression =
                        std::make_unique<ir::FieldAccess>(record_access.clone(), std::move(fresh_variable_access));
            } else {
                throw std::logic_error("Not implemented yet.");
            }
            assert(encoded_expression != nullptr);

            std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
                    std::make_unique<ir::AssignmentInstruction>(instruction.getVariableReference().clone(),
                                                                std::move(encoded_expression),
                                                                ir::AssignmentInstruction::Type::PARAMETER_OUT);

            insertInstructionAtLabel(std::move(assignment_instruction), _label);
            break;
        }
        default:
            throw std::runtime_error("Unexpected assignment instruction type encountered.");
    }
}

void CallTransformationPass::visit(const ir::CallInstruction &instruction) {
    const ir::VariableAccess &variable_access = instruction.getVariableAccess();
    std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
    const ir::DataType &data_type = variable->getDataType();
    assert(data_type.getKind() == ir::DataType::Kind::DERIVED_TYPE);
    std::string type_representative_name = data_type.getName();
    const std::shared_ptr<Cfg> &cfg = _type_representative_name_to_cfg.at(type_representative_name);

    insertInstructionAtLabel(instruction.clone(), _label);
}

void CallTransformationPass::visit(const ir::IfInstruction &instruction) {
    insertInstructionAtLabel(instruction.clone(), _label);
}

void CallTransformationPass::visit(const ir::SequenceInstruction &instruction) {
    for (auto it = instruction.instructionsBegin(); it != instruction.instructionsEnd(); ++it) {
        it->accept(*this);
    }
}

void CallTransformationPass::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void CallTransformationPass::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void CallTransformationPass::visit(const ir::HavocInstruction &instruction) {
    insertInstructionAtLabel(instruction.clone(), _label);
}

void CallTransformationPass::insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label,
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