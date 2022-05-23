#ifndef AHORN_PROJECT_H
#define AHORN_PROJECT_H

#include "ir/module.h"

#include <boost/iterator/transform_iterator.hpp>

#include <memory>
#include <string>
#include <unordered_map>

class Project {
private:
    using module_map_t = std::map<std::string, std::unique_ptr<ir::Module>>;

public:
    explicit Project(module_map_t name_to_module);

private:
    struct getModuleReference : std::unary_function<module_map_t::value_type, ir::Module> {
        getModuleReference() = default;
        ir::Module &operator()(const module_map_t::value_type &pair) const {
            return *(pair.second);
        }
    };

public:
    using const_module_it = boost::transform_iterator<getModuleReference, module_map_t::const_iterator>;
    const_module_it modulesBegin() const;
    const_module_it modulesEnd() const;

    // Returns true, if a module for the given name exists, else returns false.
    bool containsModule(const std::string &name) const;

    // Retrieves a module given the name of a type representative pou.
    const ir::Module &getModule(const std::string &name) const;

    // Retrieves the main module or throws an exception if it does not exist.
    const ir::Module &getMain() const;

private:
    const module_map_t _name_to_module;
};

#endif//AHORN_PROJECT_H
