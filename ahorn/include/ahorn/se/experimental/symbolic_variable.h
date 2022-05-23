#ifndef AHORN_SYMBOLIC_VARIABLE_H
#define AHORN_SYMBOLIC_VARIABLE_H

#include <string>

namespace se {
    class SymbolicVariable {
    public:
        // XXX default constructor disabled
        SymbolicVariable() = delete;
        // XXX copy constructor disabled
        SymbolicVariable(const SymbolicVariable &other) = delete;
        // XXX copy assignment disabled
        SymbolicVariable &operator=(const SymbolicVariable &) = delete;

        SymbolicVariable(std::string flattened_name, unsigned int version, unsigned int cycle);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const SymbolicVariable &state) {
            return state.print(os);
        }

    private:
        const std::string _flattened_name;
        const unsigned int _version;
        const unsigned int _cycle;
    };
}// namespace se

#endif//AHORN_SYMBOLIC_VARIABLE_H
