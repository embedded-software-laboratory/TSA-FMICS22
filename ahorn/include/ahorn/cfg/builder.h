#ifndef AHORN_BUILDER_H
#define AHORN_BUILDER_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction_visitor.h"
#include "ir/project.h"

#include <map>
#include <memory>
#include <vector>

class Builder : private ir::InstructionVisitor {
private:
    using cfg_map_t = std::map<std::string, std::shared_ptr<Cfg>>;
    using variable_vector_t = std::vector<std::shared_ptr<ir::Variable>>;
    using variable_map_t = std::map<std::string, std::unique_ptr<ir::Variable>>;
    using vertex_t = Cfg::vertex_t;
    using vertex_map_t = Cfg::vertex_map_t;
    using edge_vector_t = Cfg::edge_vector_t;

public:
    explicit Builder(const Project &project) : _project(project){};

    std::shared_ptr<Cfg> build();

private:
    void build(const ir::Module &module);

    const Project &_project;
    // the fully qualified name of the currently build cfg, used for augmenting interprocedural return edges
    std::string _fully_qualified_name;
    std::unique_ptr<ir::Interface> _interface;
    variable_map_t _name_to_flattened_variable;

    int _label;

    vertex_map_t _label_to_vertex;
    edge_vector_t _edges;

    cfg_map_t _name_to_cfg;

    void visit(const ir::AssignmentInstruction &instruction) override;
    void visit(const ir::CallInstruction &instruction) override;
    void visit(const ir::IfInstruction &instruction) override;
    void visit(const ir::SequenceInstruction &instruction) override;
    void visit(const ir::WhileInstruction &instruction) override;
    void visit(const ir::GotoInstruction &instruction) override;
    void visit(const ir::HavocInstruction &instruction) override;
};

#endif//AHORN_BUILDER_H
