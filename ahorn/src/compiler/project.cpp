#include "compiler/project.h"

#include <algorithm>

using namespace compiler;

Project::Project(Project::pou_map_t name_to_pou, Project::data_type_map_t name_to_data_type)
    : _name_to_pou(std::move(name_to_pou)), _name_to_data_type(std::move(name_to_data_type)) {}


Project::const_pou_it Project::pousBegin() const {
    return boost::make_transform_iterator(_name_to_pou.begin(), getPouReference());
}

Project::const_pou_it Project::pousEnd() const {
    return boost::make_transform_iterator(_name_to_pou.end(), getPouReference());
}

const Pou &Project::getProgram() const {
    auto it = std::find_if(_name_to_pou.begin(), _name_to_pou.end(),
                           [](const auto &p) { return (p.second->getKind() == Pou::Kind::PROGRAM); });
    if (it == _name_to_pou.end()) {
        throw std::logic_error("Project does not contain a program.");
    } else {
        return *(it->second);
    }
}

const Pou &Project::getPou(const std::string &name) const {
    auto it = _name_to_pou.find(name);
    if (it == _name_to_pou.end()) {
        throw std::logic_error("No pou for given name exists.");
    } else {
        return *(it->second);
    }
}
