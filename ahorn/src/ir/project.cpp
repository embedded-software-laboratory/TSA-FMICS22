#include "ir/project.h"

#include <algorithm>

Project::Project(Project::module_map_t name_to_module) : _name_to_module(std::move(name_to_module)) {}

Project::const_module_it Project::modulesBegin() const {
    return boost::make_transform_iterator(_name_to_module.begin(), getModuleReference());
}

Project::const_module_it Project::modulesEnd() const {
    return boost::make_transform_iterator(_name_to_module.end(), getModuleReference());
}

bool Project::containsModule(const std::string &name) const {
    return !(_name_to_module.find(name) == _name_to_module.end());
}

const ir::Module &Project::getModule(const std::string &name) const {
    if (containsModule(name)) {
        return *_name_to_module.at(name);
    } else {
        throw std::logic_error("No module for given name exists.");
    }
}

const ir::Module &Project::getMain() const {
    auto it = std::find_if(_name_to_module.begin(), _name_to_module.end(),
                           [](const auto &it) { return it.second->getKind() == ir::Module::Kind::PROGRAM; });
    if (it == _name_to_module.end()) {
        throw std::logic_error("No main module found in project.");
    } else {
        return *(it->second);
    }
}
