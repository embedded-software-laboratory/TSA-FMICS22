#ifndef AHORN_POU_H
#define AHORN_POU_H

#include "compiler/pou/body.h"
#include "compiler/statement/statement.h"
#include "ir/instruction/instruction.h"
#include "ir/interface.h"
#include "ir/name.h"
#include "ir/variable.h"
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

class Pou : public ir::Name {
private:
    using pou_ref_t = std::reference_wrapper<const Pou>;
    using pou_ref_map_t = std::map<std::string, pou_ref_t>;
    using variable_map_t = std::map<std::string, std::shared_ptr<ir::Variable>>;

public:
    enum class Kind { PROGRAM = 0, FUNCTION_BLOCK = 1, FUNCTION = 2 };
    Pou(std::string name, Kind kind, std::unique_ptr<ir::Interface> interface, pou_ref_map_t name_to_pou,
        std::unique_ptr<Body> body);
    Kind getKind() const;
    ir::Interface &getInterface() const;
    const Body &getBody() const;

    friend std::ostream &operator<<(std::ostream &os, Pou const &pou) { return pou.print(os); }
    std::ostream &print(std::ostream &os) const;

    std::shared_ptr<ir::Variable> getVariable(const std::string &name) const;
    std::shared_ptr<ir::Variable> getVariable(const ir::VariableReference &variable_reference) const;

private:
    struct getPouReference : std::unary_function<pou_ref_map_t::value_type, Pou> {
        getPouReference() = default;
        const Pou &operator()(const pou_ref_map_t::value_type &p) const { return p.second.get(); }
    };

private:
    using const_pou_ref_it = boost::transform_iterator<getPouReference, pou_ref_map_t::const_iterator>;

public:
    const_pou_ref_it pousBegin() const;
    const_pou_ref_it pousEnd() const;

private:
    void updateFlattenedInterface();

private:
    struct getVariableReference : std::unary_function<variable_map_t::value_type, ir::Variable> {
        getVariableReference() = default;
        ir::Variable &operator()(const variable_map_t::value_type &p) const { return *(p.second); }
    };

private:
    using const_variable_it = boost::transform_iterator<getVariableReference, variable_map_t::const_iterator>;

public:
    const_variable_it flattenedInterfaceBegin() const;
    const_variable_it flattenedInterfaceEnd() const;

private:
    const Kind _kind;
    const std::unique_ptr<ir::Interface> _interface;
    variable_map_t _name_to_flattened_variable;
    const pou_ref_map_t _name_to_pou;
    const std::unique_ptr<Body> _body;
};

#endif//AHORN_POU_H
