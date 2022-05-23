#ifndef AHORN_VERTEX_H
#define AHORN_VERTEX_H

#include "ir/instruction/instruction.h"

#include <memory>

class Vertex {
public:
    enum class Type { ENTRY = 1, REGULAR = 2, EXIT = 3 };

    // XXX default constructor disabled
    Vertex() = delete;
    // XXX copy constructor disabled
    Vertex(const Vertex &other) = delete;
    // XXX copy assignment disabled
    Vertex &operator=(const Vertex &) = delete;

    explicit Vertex(unsigned int label, Type type = Type::REGULAR);

    Vertex(unsigned int label, std::unique_ptr<ir::Instruction> instruction);

    std::ostream &print(std::ostream &os) const;

    friend std::ostream &operator<<(std::ostream &os, const Vertex &vertex) {
        return vertex.print(os);
    }

    ir::Instruction *getInstruction() const;
    void setInstruction(std::unique_ptr<ir::Instruction> instruction);

    unsigned int getLabel() const;
    void setType(Type type);
    Type getType() const;

private:
    unsigned int _label;
    std::unique_ptr<ir::Instruction> _instruction;
    Type _type;
};

class VertexLabelComparator {
public:
    // Comparator for Vertices to work with standard containers. Sort them by label
    bool operator()(const std::shared_ptr<Vertex> &v1, const std::shared_ptr<Vertex> &v2) const {
        return v1->getLabel() < v2->getLabel();
    }
};

#endif//AHORN_VERTEX_H
