#include "pass/tac/tac_pass.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/unary_expression.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"
#include "ir/type/elementary_type.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace pass;

const std::string TacPass::TEMPORARY_VARIABLE_NAME_PREFIX = "temp";

TacPass::TacPass() : _encoder(std::make_unique<Encoder>(*this)) {}

std::shared_ptr<Cfg> TacPass::apply(const Cfg &cfg) {
    auto logger = spdlog::get("Tac");

    std::map<std::string, std::shared_ptr<Cfg>> type_representative_name_to_cfg;
    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        std::string type_representative_name = callee_it->getName();
        auto it = _type_representative_name_to_cfg.find(type_representative_name);
        if (it == _type_representative_name_to_cfg.end()) {
            std::shared_ptr<Cfg> callee_cfg = apply(*callee_it);
            _temporary_variables.clear();
            _label_to_instructions.clear();
            _type_representative_name_to_cfg.emplace(type_representative_name, callee_cfg);
            type_representative_name_to_cfg.emplace(type_representative_name, callee_cfg);
        } else {
            type_representative_name_to_cfg.emplace(type_representative_name, it->second);
        }
    }

    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        switch (it->getType()) {
            case Vertex::Type::ENTRY: {
                break;
            }
            case Vertex::Type::REGULAR: {
                _label = it->getLabel();
                ir::Instruction *instruction = it->getInstruction();
                assert(instruction != nullptr);
                instruction->accept(*this);
                break;
            }
            case Vertex::Type::EXIT: {
                break;
            }
            default:
                throw std::runtime_error("Unexpected vertex type encountered.");
        }
    }

    {
        std::stringstream str;
        for (auto it = _temporary_variables.begin(); it != _temporary_variables.end(); ++it) {
            str << **it;
            if (std::next(it) != _temporary_variables.end()) {
                str << ", ";
            }
        }
        SPDLOG_LOGGER_TRACE(logger, "Temporary variables: [{}]", str.str());
    }

    {
        for (const auto &label_to_instructions : _label_to_instructions) {
            std::stringstream str;
            str << "label: " << label_to_instructions.first;
            str << " [";
            for (auto it = label_to_instructions.second.begin(); it != label_to_instructions.second.end(); ++it) {
                str << **it;
                if (std::next(it) != label_to_instructions.second.end()) {
                    str << " ";
                }
            }
            str << "]";
            SPDLOG_LOGGER_TRACE(logger, "{}", str.str());
        }
    }

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
    // augment variables with introduced temporary variables
    for (auto &_temporary_variable : _temporary_variables) {
        variables.push_back(_temporary_variable);
    }

    // build basic blocks and edge relation
    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        unsigned int label = it->getLabel();
        if (label == cfg.getEntryLabel() || label == cfg.getExitLabel()) {
            label_to_vertex.emplace(label, std::make_shared<Vertex>(label, it->getType()));
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

    return std::make_shared<Cfg>(cfg.getType(), cfg.getName(), std::move(interface),
                                 std::move(type_representative_name_to_cfg), std::move(label_to_vertex),
                                 std::move(edges), cfg.getEntryLabel(), cfg.getExitLabel());
}

std::unique_ptr<ir::VariableAccess>
TacPass::introduceTemporaryVariableExpression(std::unique_ptr<ir::Expression> expression) {
    std::string temporary_variable_name = TEMPORARY_VARIABLE_NAME_PREFIX + std::to_string(_value++);
    // TODO (24.02.2022): Implement a proper semantic analyzer in the compiler.
    std::unique_ptr<ir::DataType> data_type;
    switch (expression->getType()) {
        case ir::Expression::Type::ARITHMETIC: {
            data_type = std::make_unique<ir::ElementaryType>("INT");
            break;
        }
        case ir::Expression::Type::BOOLEAN: {
            data_type = std::make_unique<ir::ElementaryType>("BOOL");
            break;
        }
        case ir::Expression::Type::UNDEFINED: {
            // XXX fall-through
        }
        default:
            throw std::runtime_error("Unexpected expression type encountered.");
    }
    assert(data_type != nullptr);

    // introduce fresh new temporary variable for the lhs
    std::shared_ptr<ir::Variable> temporary_variable = std::make_shared<ir::Variable>(
            temporary_variable_name, std::move(data_type), ir::Variable::StorageType::TEMPORARY);
    _temporary_variables.push_back(temporary_variable);

    // introduce temporary variable expression for substitution
    std::unique_ptr<ir::VariableAccess> temporary_variable_expression =
            std::make_unique<ir::VariableAccess>(temporary_variable);

    // introduce temporary assignment, where the lhs is the temporary variable and the rhs is the left expression
    std::unique_ptr<ir::AssignmentInstruction> assignment_instruction =
            std::make_unique<ir::AssignmentInstruction>(temporary_variable_expression->clone(), std::move(expression));
    insertInstructionAtLabel(std::move(assignment_instruction), _label);

    return temporary_variable_expression;
}

void TacPass::insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label) {
    if (_label_to_instructions.find(label) == _label_to_instructions.end()) {
        std::vector<std::unique_ptr<ir::Instruction>> instructions;
        instructions.push_back(std::move(instruction));
        _label_to_instructions.emplace(label, std::move(instructions));
    } else {
        _label_to_instructions.at(label).push_back(std::move(instruction));
    }
}

void TacPass::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();
    std::unique_ptr<ir::Expression> encoded_expression = _encoder->encode(expression);

    std::unique_ptr<ir::AssignmentInstruction> assignment_instruction = std::make_unique<ir::AssignmentInstruction>(
            instruction.getVariableReference().clone(), std::move(encoded_expression), instruction.getType());

    insertInstructionAtLabel(std::move(assignment_instruction), _label);
}

void TacPass::visit(const ir::CallInstruction &instruction) {
    // XXX calls are split-up into pre-/post-assignments and the actual call. The pre-/post-assignments are handled
    // XXX separately, hence, keep the call instruction at the corresponding block without any modification.
    insertInstructionAtLabel(instruction.clone(), _label);
}

void TacPass::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();
    std::unique_ptr<ir::Expression> encoded_expression = _encoder->encode(expression);

    std::unique_ptr<ir::IfInstruction> if_instruction = nullptr;
    if (encoded_expression->getKind() == ir::Expression::Kind::UNARY_EXPRESSION ||
        encoded_expression->getKind() == ir::Expression::Kind::BINARY_EXPRESSION) {
        std::unique_ptr<ir::VariableAccess> temporary_variable_expression =
                introduceTemporaryVariableExpression(std::move(encoded_expression));
        if_instruction = std::make_unique<ir::IfInstruction>(std::move(temporary_variable_expression),
                                                             instruction.getGotoThen().clone(),
                                                             instruction.getGotoElse().clone());
    } else {
        if_instruction = std::make_unique<ir::IfInstruction>(
                std::move(encoded_expression), instruction.getGotoThen().clone(), instruction.getGotoElse().clone());
    }
    insertInstructionAtLabel(std::move(if_instruction), _label);
}

void TacPass::visit(const ir::SequenceInstruction &instruction) {
    for (auto it = instruction.instructionsBegin(); it != instruction.instructionsEnd(); ++it) {
        it->accept(*this);
    }
}

void TacPass::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void TacPass::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void TacPass::visit(const ir::HavocInstruction &instruction) {
    // XXX havoc(variable) is always in "TAC-form"
    insertInstructionAtLabel(instruction.clone(), _label);
}
