#ifndef AHORN_COMPILER_PROJECT_H
#define AHORN_COMPILER_PROJECT_H

#include "compiler/pou/pou.h"
#include "ir/type/data_type.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace compiler {
    class Project {
    private:
        using pou_map_t = std::map<std::string, std::unique_ptr<Pou>>;
        using data_type_map_t = std::map<std::string, std::unique_ptr<ir::DataType>>;

    public:
        Project(pou_map_t name_to_pou, data_type_map_t name_to_data_type);

    private:
        struct getPouReference : std::unary_function<pou_map_t::value_type, Pou> {
            getPouReference() = default;
            Pou &operator()(const pou_map_t::value_type &pair) const {
                return *(pair.second);
            }
        };

    public:
        using const_pou_it = boost::transform_iterator<getPouReference, pou_map_t::const_iterator>;
        const_pou_it pousBegin() const;
        const_pou_it pousEnd() const;

        // Retrieves a type representative pou given its name.
        const Pou &getPou(const std::string &name) const;

        const Pou &getProgram() const;

    private:
        pou_map_t _name_to_pou;
        data_type_map_t _name_to_data_type;
    };
}// namespace compiler

#endif//AHORN_COMPILER_PROJECT_H
