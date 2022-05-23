#include "sa/crab/marshaller.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/field_access.h"
#include "ir/expression/variable_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/sequence_instruction.h"
#include "ir/type/elementary_type.h"
#include "ir/type/safety_type.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace sa;

const std::string Marshaller::BASIC_BLOCK_NAME_PREFIX = "bb_";

Marshaller::Marshaller() : _encoder(std::make_unique<Encoder>(*this)), _crab_variable_factory() {}

cfg_t *Marshaller::marshall(const Cfg &cfg) {
    auto logger = spdlog::get("Static Analysis");

    if (cfg.getType() == Cfg::Type::PROGRAM) {
        // XXX create cycle variable, passed in- and out of callees (necessary for the boxes domain)
        crab::variable_type cycle_variable_type(crab::variable_type_kind::INT_TYPE, 16);
        var_t cycle_variable(_crab_variable_factory["cycle"], cycle_variable_type);
        _name_to_crab_variable.emplace("cycle", cycle_variable);
    }

    // recurse on callees
    for (auto callee_it = cfg.calleesBegin(); callee_it != cfg.calleesEnd(); ++callee_it) {
        std::string type_representative_name = callee_it->getName();
        auto it = _type_representative_name_to_crab_cfg.find(type_representative_name);
        if (it == _type_representative_name_to_crab_cfg.end()) {
            cfg_t *callee_cfg = marshall(*callee_it);
            _type_representative_name_to_crab_cfg.emplace(type_representative_name, callee_cfg);
        }
    }

    _cfg = &cfg;
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.variablesBegin(); it != interface.variablesEnd(); ++it) {
        std::string name = it->getName();
        const ir::DataType &data_type = it->getDataType();
        switch (data_type.getKind()) {
            case ir::DataType::Kind::ELEMENTARY_TYPE: {
                auto *elementary_type = dynamic_cast<const ir::ElementaryType *>(&data_type);
                assert(elementary_type != nullptr);
                switch (elementary_type->getType()) {
                    case ir::ElementaryType::Type::INT: {
                        crab::variable_type crab_variable_type(crab::variable_type_kind::INT_TYPE, 16);
                        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
                        _name_to_crab_variable.emplace(name, crab_variable);
                        break;
                    }
                    case ir::ElementaryType::Type::REAL: {
                        throw std::runtime_error("Unsupported elementary type.");
                    }
                    case ir::ElementaryType::Type::BOOL: {
                        crab::variable_type crab_variable_type(crab::variable_type_kind::BOOL_TYPE);
                        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
                        _name_to_crab_variable.emplace(name, crab_variable);
                        break;
                    }
                    case ir::ElementaryType::Type::TIME: {
                        crab::variable_type crab_variable_type(crab::variable_type_kind::INT_TYPE, 16);
                        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
                        _name_to_crab_variable.emplace(name, crab_variable);
                        break;
                    }
                    default:
                        throw std::runtime_error("Unexpected elementary type encountered.");
                }
                break;
            }
            case ir::DataType::Kind::SAFETY_TYPE: {
                auto *safety_type = dynamic_cast<const ir::SafetyType *>(&data_type);
                assert(safety_type != nullptr);
                switch (safety_type->getType()) {
                    case ir::SafetyType::Kind::SAFEBOOL: {
                        crab::variable_type crab_variable_type(crab::variable_type_kind::BOOL_TYPE);
                        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
                        _name_to_crab_variable.emplace(name, crab_variable);
                        break;
                    }
                    default:
                        throw std::runtime_error("Unexpected safety type encountered.");
                }
                break;
            }
            case ir::DataType::Kind::DERIVED_TYPE:
                break;
            case ir::DataType::Kind::ENUMERATED_TYPE:
                break;
            case ir::DataType::Kind::INCONCLUSIVE_TYPE:
                break;
            case ir::DataType::Kind::SIMPLE_TYPE:
                break;
            default:
                throw std::runtime_error("Unexpected data type encountered.");
        }
    }

    std::string type_representative_name = cfg.getName();
    std::vector<var_t> input_variables;
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string name = it->getName();
        var_t crab_variable = _name_to_crab_variable.at(name);
        input_variables.push_back(crab_variable);
    }
    if (cfg.getType() != Cfg::Type::PROGRAM) {
        std::string name = cfg.getName() + "_cycle_in";
        crab::variable_type crab_variable_type(crab::variable_type_kind::INT_TYPE, 16);
        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
        _name_to_crab_variable.emplace(name, crab_variable);
        input_variables.push_back(crab_variable);
    }
    std::vector<var_t> output_variables;
    for (auto it = interface.outputVariablesBegin(); it != interface.outputVariablesEnd(); ++it) {
        std::string name = it->getName();
        var_t crab_variable = _name_to_crab_variable.at(name);
        output_variables.push_back(crab_variable);
    }
    if (cfg.getType() != Cfg::Type::PROGRAM) {
        std::string name = cfg.getName() + "_cycle_out";
        crab::variable_type crab_variable_type(crab::variable_type_kind::INT_TYPE, 16);
        var_t crab_variable(_crab_variable_factory[name], crab_variable_type);
        _name_to_crab_variable.emplace(name, crab_variable);
        output_variables.push_back(crab_variable);
    }

    if (cfg.getType() == Cfg::Type::PROGRAM) {
        interface_declaration_t interface_declaration(type_representative_name, {}, {});
        _crab_cfg = new cfg_t("initialization", "exit", interface_declaration);
        var_t cycle = _name_to_crab_variable.at("cycle");
        // create basic blocks for cycle semantics
        basic_block_t &initialization = _crab_cfg->insert("initialization");
        basic_block_t &entry = _crab_cfg->insert("entry");
        basic_block_t &cycle_entry = _crab_cfg->insert("cycle_entry");
        basic_block_t &cycle_exit = _crab_cfg->insert("cycle_exit");
        basic_block_t &exit = _crab_cfg->insert("exit");
        // create control-flow for cycle semantics
        initialization.add_succ(entry);
        initialization.assign(cycle, 0);
        entry.add_succ(cycle_entry);
        cycle_exit.add_succ(entry);
        cycle_exit.add_succ(exit);
        // create cycle statements
        for (auto it = interface.variablesBegin(); it != interface.variablesEnd(); ++it) {
            if (it->getStorageType() == ir::Variable::StorageType::INPUT) {
                // XXX input variables are explicitly havoc'ed on cycle entry
                continue;
            }
            std::string name = it->getName();
            const ir::DataType &data_type = it->getDataType();
            switch (data_type.getKind()) {
                case ir::DataType::Kind::ELEMENTARY_TYPE: {
                    auto *elementary_type = dynamic_cast<const ir::ElementaryType *>(&data_type);
                    assert(elementary_type != nullptr);
                    switch (elementary_type->getType()) {
                        case ir::ElementaryType::Type::INT: {
                            var_t crab_variable = _name_to_crab_variable.at(name);
                            if (it->hasInitialization()) {
                                const ir::Constant &constant = it->getInitialization();
                                auto *integer_constant = dynamic_cast<const ir::IntegerConstant *>(&constant);
                                assert(integer_constant != nullptr);
                                initialization.assign(crab_variable, integer_constant->getValue());
                            } else {
                                initialization.assign(crab_variable, 0);
                            }
                            break;
                        }
                        case ir::ElementaryType::Type::REAL: {
                            throw std::runtime_error("Unsupported elementary type.");
                        }
                        case ir::ElementaryType::Type::BOOL: {
                            var_t crab_variable = _name_to_crab_variable.at(name);
                            if (it->hasInitialization()) {
                                const ir::Constant &constant = it->getInitialization();
                                auto *boolean_constant = dynamic_cast<const ir::BooleanConstant *>(&constant);
                                assert(boolean_constant != nullptr);
                                initialization.bool_assign(crab_variable, boolean_constant->getValue()
                                                                                  ? lin_cst_t::get_true()
                                                                                  : lin_cst_t::get_false());
                            } else {
                                initialization.bool_assign(crab_variable, lin_cst_t::get_false());
                            }
                            break;
                        }
                        case ir::ElementaryType::Type::TIME: {
                            var_t crab_variable = _name_to_crab_variable.at(name);
                            if (it->hasInitialization()) {
                                throw std::logic_error("Not implemented yet.");
                            } else {
                                initialization.assign(crab_variable, 0);
                            }
                            break;
                        }
                        default:
                            throw std::runtime_error("Unexpected elementary type encountered.");
                    }
                    break;
                }
                case ir::DataType::Kind::SAFETY_TYPE: {
                    auto *safety_type = dynamic_cast<const ir::SafetyType *>(&data_type);
                    assert(safety_type != nullptr);
                    switch (safety_type->getType()) {
                        case ir::SafetyType::Kind::SAFEBOOL: {
                            var_t crab_variable = _name_to_crab_variable.at(name);
                            if (it->hasInitialization()) {
                                throw std::logic_error("Not implemented yet.");
                            } else {
                                initialization.bool_assign(crab_variable, lin_cst_t::get_false());
                            }
                            break;
                        }
                        default:
                            throw std::runtime_error("Unexpected safety type encountered.");
                    }
                    break;
                }
                case ir::DataType::Kind::DERIVED_TYPE:
                    break;
                case ir::DataType::Kind::ENUMERATED_TYPE:
                    break;
                case ir::DataType::Kind::INCONCLUSIVE_TYPE:
                    break;
                case ir::DataType::Kind::SIMPLE_TYPE:
                    break;
                default:
                    throw std::runtime_error("Unexpected data type encountered.");
            }
        }
        entry.assume(cycle < 100);
        cycle_entry.add(cycle, cycle, 1);
        for (auto it = interface.variablesBegin(); it != interface.variablesEnd(); ++it) {
            if (it->getStorageType() == ir::Variable::StorageType::INPUT) {
                std::string name = it->getName();
                var_t crab_variable = _name_to_crab_variable.at(name);
                cycle_entry.havoc(crab_variable);
            }
        }
        exit.assume(cycle >= 100);

        // XXX collect the lowest and highest versions of the local and output variables to couple their valuations
        // XXX into the next cycle
        std::map<std::string, std::pair<unsigned int, unsigned int>> name_to_lowest_and_highest_value;
        for (auto it = interface.localVariablesBegin(); it != interface.localVariablesEnd(); ++it) {
            std::string name = it->getName();
            if (it->getDataType().getKind() == ir::DataType::Kind::DERIVED_TYPE) {
                // XXX ignore FBs
                continue;
            }
            std::size_t value_position = name.find('_');
            std::string unvalued_name = name.substr(0, value_position);
            unsigned int value = std::stoi(name.substr(value_position + 1, name.length()));
            if (name_to_lowest_and_highest_value.find(unvalued_name) == name_to_lowest_and_highest_value.end()) {
                name_to_lowest_and_highest_value.emplace(unvalued_name, std::pair(value, value));
            } else {
                if (value < name_to_lowest_and_highest_value.at(unvalued_name).first) {
                    name_to_lowest_and_highest_value.at(unvalued_name).first = value;
                }
                if (value > name_to_lowest_and_highest_value.at(unvalued_name).second) {
                    name_to_lowest_and_highest_value.at(unvalued_name).second = value;
                }
            }
        }
        for (auto it = interface.outputVariablesBegin(); it != interface.outputVariablesEnd(); ++it) {
            std::string name = it->getName();
            std::size_t value_position = name.find('_');
            std::string unvalued_name = name.substr(0, value_position);
            unsigned int value = std::stoi(name.substr(value_position + 1, name.length()));
            if (name_to_lowest_and_highest_value.find(unvalued_name) == name_to_lowest_and_highest_value.end()) {
                name_to_lowest_and_highest_value.emplace(unvalued_name, std::pair(value, value));
            } else {
                if (value < name_to_lowest_and_highest_value.at(unvalued_name).first) {
                    name_to_lowest_and_highest_value.at(unvalued_name).first = value;
                }
                if (value > name_to_lowest_and_highest_value.at(unvalued_name).second) {
                    name_to_lowest_and_highest_value.at(unvalued_name).second = value;
                }
            }
        }

        for (const auto &value_pairs : name_to_lowest_and_highest_value) {
            var_t lowest_value_variable =
                    _name_to_crab_variable.at(value_pairs.first + "_" + std::to_string(value_pairs.second.first));
            var_t highest_value_variable =
                    _name_to_crab_variable.at(value_pairs.first + "_" + std::to_string(value_pairs.second.second));
            if (lowest_value_variable.get_type() == crab::variable_type(crab::variable_type_kind::INT_TYPE, 16)) {
                assert(highest_value_variable.get_type() ==
                       crab::variable_type(crab::variable_type_kind::INT_TYPE, 16));
                cycle_exit.assign(lowest_value_variable, highest_value_variable);
            } else if (lowest_value_variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE)) {
                assert(highest_value_variable.get_type() == crab::variable_type(crab::variable_type_kind::BOOL_TYPE));
                cycle_exit.bool_assign(lowest_value_variable, highest_value_variable);
            } else {
                throw std::runtime_error("Unexpected variable type.");
            }
        }
    } else {
        basic_block_label_t entry = BASIC_BLOCK_NAME_PREFIX + std::to_string(cfg.getEntryLabel());
        basic_block_label_t exit = BASIC_BLOCK_NAME_PREFIX + std::to_string(cfg.getExitLabel());
        interface_declaration_t interface_declaration(type_representative_name, input_variables, output_variables);
        _crab_cfg = new cfg_t(entry, exit, interface_declaration);
        _crab_cfg->insert(entry);
        _crab_cfg->insert(exit);
    }

    // create crab basic blocks
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        unsigned int label = it->getLabel();
        if (label == cfg.getEntryLabel() || label == cfg.getExitLabel()) {
            // entry and exit basic blocks are explicitly created
            continue;
        }
        _crab_cfg->insert(BASIC_BLOCK_NAME_PREFIX + std::to_string(label));
        const ir::Instruction *instruction = it->getInstruction();
        assert(instruction != nullptr);
        switch (instruction->getKind()) {
            case ir::Instruction::Kind::IF: {
                // XXX fall-through
            }
            case ir::Instruction::Kind::WHILE: {
                _crab_cfg->insert(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_tt");
                _crab_cfg->insert(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_ff");
                break;
            }
            case ir::Instruction::Kind::SEQUENCE: {
                auto *sequence_instruction = dynamic_cast<const ir::SequenceInstruction *>(instruction);
                assert(sequence_instruction != nullptr);
                if (sequence_instruction->containsIfInstruction()) {
                    _crab_cfg->insert(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_tt");
                    _crab_cfg->insert(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_ff");
                }
                break;
            }
            // XXX do nothing for the other cases as they do not need additional instrumentation
            case ir::Instruction::Kind::ASSIGNMENT:
            case ir::Instruction::Kind::HAVOC:
            case ir::Instruction::Kind::CALL:
            case ir::Instruction::Kind::GOTO:
                break;
            default:
                throw std::runtime_error("Unexpected instruction kind encountered.");
        }
    }

    // create crab control-flow relation
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        unsigned int label = it->getLabel();
        if (cfg.getType() == Cfg::Type::PROGRAM && (label == cfg.getEntryLabel() || label == cfg.getExitLabel())) {
            // XXX control-flow relation for entry and exit of the main cfg is explicitly handled
            continue;
        }
        basic_block_t &bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(label));
        for (const auto &outgoing_edge : cfg.getOutgoingEdges(label)) {
            switch (outgoing_edge->getType()) {
                case Edge::Type::INTRAPROCEDURAL: {
                    unsigned int next_label = outgoing_edge->getTargetLabel();
                    if (cfg.getType() == Cfg::Type::PROGRAM && next_label == cfg.getExitLabel()) {
                        // XXX labels to exit are handled explicitly for the main cfg to allow for cycle semantics
                        continue;
                    }
                    basic_block_t &next_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(next_label));
                    bb.add_succ(next_bb);
                    break;
                }
                case Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN: {
                    unsigned int next_label = outgoing_edge->getTargetLabel();
                    if (cfg.getType() == Cfg::Type::PROGRAM && next_label == cfg.getExitLabel()) {
                        // XXX labels to exit are handled explicitly for the main cfg to allow for cycle semantics
                        continue;
                    }
                    basic_block_t &next_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(next_label));
                    bb.add_succ(next_bb);
                    break;
                }
                case Edge::Type::INTERPROCEDURAL_CALL: {
                    // XXX skip, implicitly handled via crab call site
                    break;
                }
                case Edge::Type::TRUE_BRANCH: {
                    basic_block_t &bb_tt = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_tt");
                    unsigned int next_label = outgoing_edge->getTargetLabel();
                    if (cfg.getType() == Cfg::Type::PROGRAM && next_label == cfg.getExitLabel()) {
                        // XXX labels to exit are handled explicitly for the main cfg to allow for cycle semantics
                        continue;
                    }
                    basic_block_t &next_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(next_label));
                    bb.add_succ(bb_tt);
                    bb_tt.add_succ(next_bb);
                    break;
                }
                case Edge::Type::FALSE_BRANCH: {
                    basic_block_t &bb_ff = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(label) + "_ff");
                    unsigned int next_label = outgoing_edge->getTargetLabel();
                    if (cfg.getType() == Cfg::Type::PROGRAM && next_label == cfg.getExitLabel()) {
                        // XXX labels to exit are handled explicitly for the main cfg to allow for cycle semantics
                        continue;
                    }
                    basic_block_t &next_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(next_label));
                    bb.add_succ(bb_ff);
                    bb_ff.add_succ(next_bb);
                    break;
                }
                case Edge::Type::INTERPROCEDURAL_RETURN: {
                    // XXX skip, implicitly handled via crab call site
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected edge type encountered.");
            }
        }
    }

    // create crab control-flow relation for the cycle semantics
    if (cfg.getType() == Cfg::Type::PROGRAM) {
        basic_block_t &cycle_entry = _crab_cfg->get_node("cycle_entry");
        for (const auto &outgoing_edge : cfg.getOutgoingEdges(cfg.getEntryLabel())) {
            assert(outgoing_edge->getType() == Edge::Type::INTRAPROCEDURAL);
            unsigned int next_label = outgoing_edge->getTargetLabel();
            basic_block_t &next_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(next_label));
            cycle_entry.add_succ(next_bb);
        }
        basic_block_t &cycle_exit = _crab_cfg->get_node("cycle_exit");
        for (const auto &incoming_edge : cfg.getIncomingEdges(cfg.getExitLabel())) {
            unsigned int prior_label = incoming_edge->getSourceLabel();
            basic_block_t &prior_bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(prior_label));
            prior_bb.add_succ(cycle_exit);
        }
    }

    // create crab statements
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        _label = it->getLabel();
        const ir::Instruction *instruction = it->getInstruction();
        if (_label == cfg.getEntryLabel() || _label == cfg.getExitLabel()) {
            // XXX distinguish between program and function block entries
            if (instruction != nullptr) {
                instruction->accept(*this);
            }
            continue;
        }
        std::cout << _label << " : " << *instruction << std::endl;
        assert(instruction != nullptr);
        instruction->accept(*this);
    }

    if (cfg.getType() != Cfg::Type::PROGRAM) {
        unsigned int exit_label = cfg.getExitLabel();
        basic_block_t &bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(exit_label));
        var_t cycle_in = _name_to_crab_variable.at(cfg.getName() + "_cycle_in");
        var_t cycle_out = _name_to_crab_variable.at(cfg.getName() + "_cycle_out");
        bb.assign(cycle_out, cycle_in);
    }

    _type_representative_name_to_crab_cfg.emplace(std::move(type_representative_name), _crab_cfg);

    return _crab_cfg;
}

var_t Marshaller::getCrabVariable(const std::string &name) const {
    auto it = _name_to_crab_variable.find(name);
    if (it == _name_to_crab_variable.end()) {
        throw std::runtime_error("No crab variable with name " + name + " was created.");
    } else {
        return it->second;
    }
}

void Marshaller::visit(const ir::AssignmentInstruction &instruction) {
    _instruction = &instruction;
    switch (instruction.getType()) {
        case ir::AssignmentInstruction::Type::REGULAR: {
            const ir::Expression &expression = instruction.getExpression();
            _encoder->encode(expression);
            break;
        }
        case ir::AssignmentInstruction::Type::PARAMETER_IN: {
            // XXX fall-through
        }
        case ir::AssignmentInstruction::Type::PARAMETER_OUT: {
            // XXX explicitly handled in ir::CallInstruction as CRAB requires full specification of input and output
            // XXX relation on respective call site
            break;
        }
        default:
            throw std::runtime_error("Unexpected instruction type encountered.");
    }
}

void Marshaller::visit(const ir::CallInstruction &instruction) {
    const ir::VariableAccess &variable_access = instruction.getVariableAccess();
    std::shared_ptr<ir::Variable> callee_variable = variable_access.getVariable();
    std::string type_representative_name = callee_variable->getDataType().getName();
    cfg_t *crab_callee_cfg = _type_representative_name_to_crab_cfg.at(type_representative_name);
    const interface_declaration_t &interface_declaration = crab_callee_cfg->get_func_decl();

    const std::vector<var_t> &input_variables = interface_declaration.get_inputs();
    const Vertex &call_vertex = _cfg->getVertex(_label);
    const ir::Instruction *call_instruction = call_vertex.getInstruction();
    std::map<unsigned int, var_t> indexed_args;
    if (auto *call_sequence_instruction = dynamic_cast<const ir::SequenceInstruction *>(call_instruction)) {
        for (auto it = call_sequence_instruction->instructionsBegin();
             it != call_sequence_instruction->instructionsEnd(); ++it) {
            if (auto *potential_input_assignment = dynamic_cast<const ir::AssignmentInstruction *>(&*it)) {
                const ir::VariableReference &variable_reference = potential_input_assignment->getVariableReference();
                if (auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
                    std::string name = field_access->getVariableAccess().getName();
                    for (unsigned int i = 0; i < interface_declaration.get_num_inputs(); ++i) {
                        if (input_variables.at(i).name().str() == name) {
                            if (auto *arg = dynamic_cast<const ir::VariableAccess *>(
                                        &potential_input_assignment->getExpression())) {
                                indexed_args.emplace(i, _name_to_crab_variable.at(arg->getName()));
                            } else {
                                throw std::logic_error("Not implemented yet: CRAB expects arg to be a variable, handle "
                                                       "this case!");
                            }
                        }
                    }
                }
            } else {
                continue;
            }
        }
    } else {
        throw std::logic_error("Not implemented yet.");
    }

    const Edge &intraprocedural_edge = _cfg->getIntraproceduralCallToReturnEdge(_label);
    const std::vector<var_t> &output_variables = interface_declaration.get_outputs();
    const Vertex &return_vertex = _cfg->getVertex(intraprocedural_edge.getTargetLabel());
    const ir::Instruction *return_instruction = return_vertex.getInstruction();
    std::map<unsigned int, var_t> indexed_lhs;
    if (auto *return_sequence_instruction = dynamic_cast<const ir::SequenceInstruction *>(return_instruction)) {
        for (auto it = return_sequence_instruction->instructionsBegin();
             it != return_sequence_instruction->instructionsEnd(); ++it) {
            if (auto *potential_output_assignment = dynamic_cast<const ir::AssignmentInstruction *>(&*it)) {
                const ir::Expression &expression = potential_output_assignment->getExpression();
                if (auto *field_access = dynamic_cast<const ir::FieldAccess *>(&expression)) {
                    std::string name = field_access->getVariableAccess().getName();
                    for (unsigned int i = 0; i < interface_declaration.get_num_outputs(); ++i) {
                        if (output_variables.at(i).name().str() == name) {
                            if (auto *lhs = dynamic_cast<const ir::VariableAccess *>(
                                        &potential_output_assignment->getVariableReference())) {
                                indexed_lhs.emplace(i, _name_to_crab_variable.at(lhs->getName()));
                            } else if (auto *lhs_field_access = dynamic_cast<const ir::FieldAccess *>(
                                               &potential_output_assignment->getVariableReference())) {
                                indexed_lhs.emplace(i, _name_to_crab_variable.at(lhs_field_access->getName()));
                            } else {
                                throw std::logic_error("Not implemented yet: CRAB expects lhs to be a variable, handle "
                                                       "this case!");
                            }
                        }
                    }
                }
            } else {
                continue;
            }
        }
    } else if (auto *return_assignment_instruction =
                       dynamic_cast<const ir::AssignmentInstruction *>(return_instruction)) {
        const ir::Expression &expression = return_assignment_instruction->getExpression();
        if (auto *field_access = dynamic_cast<const ir::FieldAccess *>(&expression)) {
            std::string name = field_access->getVariableAccess().getName();
            for (unsigned int i = 0; i < interface_declaration.get_num_outputs(); ++i) {
                if (output_variables.at(i).name().str() == name) {
                    if (auto *lhs = dynamic_cast<const ir::VariableAccess *>(
                                &return_assignment_instruction->getVariableReference())) {
                        indexed_lhs.emplace(i, _name_to_crab_variable.at(lhs->getName()));
                    } else {
                        throw std::logic_error("Not implemented yet: CRAB expects lhs to be a variable, handle "
                                               "this case!");
                    }
                }
            }
        }
    } else {
        // XXX ignore
    }

    basic_block_t &bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(_label));
    // XXX crab defines call sites as: x_1, ..., x_n := callee(v_1, ..., v_m);, where x_1, ..., x_n are the variables
    // XXX written by the outputs in the scope of the caller and v_1, ..., v_m are the arguments passed to the input
    // XXX variables in the scope of the callee.
    std::vector<var_t> lhs;
    lhs.reserve(indexed_lhs.size());
    for (auto &indexed_lh : indexed_lhs) {
        lhs.push_back(indexed_lh.second);
    }
    lhs.push_back(_name_to_crab_variable.at("cycle"));
    std::vector<var_t> args;
    args.reserve(indexed_args.size());
    for (auto &indexed_arg : indexed_args) {
        args.push_back(indexed_arg.second);
    }
    args.push_back(_name_to_crab_variable.at("cycle"));
    bb.callsite(type_representative_name, lhs, args);
}

void Marshaller::visit(const ir::IfInstruction &instruction) {
    _instruction = &instruction;
    const ir::Expression &expression = instruction.getExpression();
    _encoder->encode(expression);
}

void Marshaller::visit(const ir::SequenceInstruction &instruction) {
    for (auto it = instruction.instructionsBegin(); it != instruction.instructionsEnd(); ++it) {
        it->accept(*this);
    }
}

void Marshaller::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Marshaller::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Marshaller::visit(const ir::HavocInstruction &instruction) {
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    var_t crab_variable = _name_to_crab_variable.at(name);
    basic_block_t &bb = _crab_cfg->get_node(BASIC_BLOCK_NAME_PREFIX + std::to_string(_label));
    bb.havoc(crab_variable);
}
