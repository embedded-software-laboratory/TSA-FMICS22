#include "cfg/builder.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/while_instruction.h"

using namespace ir;

std::shared_ptr<Cfg> Builder::build() {
    // build a cfg for each type representative module
    for (auto it = _project.modulesBegin(); it != _project.modulesEnd(); ++it) {
        build(*it);
    }
    return _name_to_cfg.at(_project.getMain().getFullyQualifiedName());
}

void Builder::build(const Module &module) {
    std::string fully_qualified_name = module.getFullyQualifiedName();
    if (_name_to_cfg.count(fully_qualified_name))
        return;
    // XXX resolve compile dependency, assumption: call graph is acyclic (PLC-specific property!)
    // XXX the order of compilation does not matter but makes compilation easier
    variable_vector_t variables;
    cfg_map_t name_to_cfg;// referenced modules are local to this particular module
    for (auto it = module.getInterface().variablesBegin(); it != module.getInterface().variablesEnd(); ++it) {
        if (it->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // Variable : DataType, e.g., f : Fb1
            std::string variable_name = it->getName();
            std::string type_representative_name = it->getDataType().getFullyQualifiedName();
            // build type representative module
            const Module &type_representative_module = _project.getModule(type_representative_name);
            build(type_representative_module);
            // XXX intended copy of shared ptr
            auto cfg = _name_to_cfg.at(type_representative_name);
            name_to_cfg.emplace(cfg->getFullyQualifiedName(), cfg);
        }
        // XXX share ownership
        variables.push_back(module.getInterface().getVariable(it->getName()));
    }
    // XXX at this point, the interface is completely compiled and can be flattened, we pass module map by copy
    // XXX exposed as a member, used for the compilation of invocations
    _interface = std::make_unique<Interface>(std::move(variables));
    Cfg::Type type;
    switch (module.getKind()) {
        case Module::Kind::PROGRAM: {
            type = Cfg::Type::PROGRAM;
            break;
        }
        case Module::Kind::FUNCTION_BLOCK: {
            type = Cfg::Type::FUNCTION_BLOCK;
            break;
        }
        case Module::Kind::FUNCTION: {
            type = Cfg::Type::FUNCTION;
            break;
        }
    }
    int entry_label = module.getEntryLabel();
    int exit_label = module.getExitLabel();
    _label_to_vertex.emplace(entry_label, std::make_shared<Vertex>(entry_label, Vertex::Type::ENTRY));
    _label_to_vertex.emplace(exit_label, std::make_shared<Vertex>(exit_label, Vertex::Type::EXIT));
    _edges.push_back(std::make_shared<Edge>(entry_label, entry_label + 1));
    // XXX expose fully qualified name to augment interprocedural call edge
    _fully_qualified_name = fully_qualified_name;
    for (auto it = module.instructionsBegin(); it != module.instructionsEnd(); ++it) {
        _label = it->first;
        auto &instruction = it->second;
        instruction->accept(*this);
    }
    std::shared_ptr<Cfg> cfg =
            std::make_shared<Cfg>(type, std::move(fully_qualified_name), std::move(_interface), name_to_cfg,
                                  std::move(_label_to_vertex), std::move(_edges), entry_label, exit_label);
    _name_to_cfg.emplace(cfg->getFullyQualifiedName(), std::move(cfg));
}

void Builder::visit(const AssignmentInstruction &instruction) {
    _label_to_vertex.emplace(_label, std::make_unique<Vertex>(_label));
    auto &vertex = _label_to_vertex.at(_label);
    vertex->setInstruction(instruction.clone());
}

void Builder::visit(const CallInstruction &instruction) {
    _label_to_vertex.emplace(_label, std::make_unique<Vertex>(_label));
    auto &vertex = _label_to_vertex.at(_label);
    vertex->setInstruction(instruction.clone());
    auto call_variable = instruction.getVariableAccess().getVariable();
    const auto &callee_cfg = _name_to_cfg.at(call_variable->getDataType().getFullyQualifiedName());
    int exit_label = callee_cfg->getExitLabel();
    // interprocedural edge from caller to callee
    _edges.push_back(
            std::make_shared<Edge>(_label, instruction.getInterproceduralLabel(), Edge::Type::INTERPROCEDURAL_CALL));
    // intraprocedural edge
    _edges.push_back(std::make_shared<Edge>(_label, instruction.getIntraproceduralLabel(),
                                            Edge::Type::INTRAPROCEDURAL_CALL_TO_RETURN));
    // interprocedural edge from callee to caller
    _edges.push_back(std::make_shared<Edge>(exit_label, instruction.getIntraproceduralLabel(), _fully_qualified_name,
                                            _label, Edge::Type::INTERPROCEDURAL_RETURN));
}

void Builder::visit(const IfInstruction &instruction) {
    _label_to_vertex.emplace(_label, std::make_unique<Vertex>(_label));
    auto &vertex = _label_to_vertex.at(_label);
    vertex->setInstruction(instruction.clone());
    _edges.push_back(std::make_shared<Edge>(_label, instruction.getThenLabel(), Edge::Type::TRUE_BRANCH));
    _edges.push_back(std::make_shared<Edge>(_label, instruction.getElseLabel(), Edge::Type::FALSE_BRANCH));
}

void Builder::visit(const SequenceInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Builder::visit(const WhileInstruction &instruction) {
    _label_to_vertex.emplace(_label, std::make_unique<Vertex>(_label));
    auto &vertex = _label_to_vertex.at(_label);
    vertex->setInstruction(instruction.clone());
    _edges.push_back(std::make_shared<Edge>(_label, instruction.getBodyEntryLabel(), Edge::Type::TRUE_BRANCH));
    _edges.push_back(std::make_shared<Edge>(_label, instruction.getWhileExitLabel(), Edge::Type::FALSE_BRANCH));
}

void Builder::visit(const GotoInstruction &instruction) {
    // XXX gotos denote explicit jumps after the preceding instruction, hence we add an edge from _call_label - 1 to the
    // XXX target of the goto instruction, however, entry vertices are always empty, i.e., do not have an instruction
    _edges.push_back(std::make_shared<Edge>(_label - 1, instruction.getLabel()));
}

void Builder::visit(const HavocInstruction &instruction) {
    _label_to_vertex.emplace(_label, std::make_unique<Vertex>(_label));
    auto &vertex = _label_to_vertex.at(_label);
    vertex->setInstruction(instruction.clone());
}