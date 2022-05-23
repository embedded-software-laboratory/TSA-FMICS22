#include "cfg/vertex.h"

#include <sstream>

using namespace ir;

Vertex::Vertex(unsigned int label, Type type)
    : _label(label), _instruction(std::unique_ptr<Instruction>()), _type(type) {}

Vertex::Vertex(unsigned int label, std::unique_ptr<ir::Instruction> instruction)
    : _label(label), _instruction(std::move(instruction)), _type(Vertex::Type::REGULAR) {}

std::ostream &Vertex::print(std::ostream &os) const {
    std::stringstream str;
    str << "{";
    str << "label: " << _label << ", ";
    str << "type: ";
    switch (_type) {
        case Type::ENTRY: {
            str << "ENTRY";
            break;
        }
        case Type::REGULAR: {
            str << "REGULAR";
            break;
        }
        case Type::EXIT: {
            str << "EXIT";
            break;
        }
        default:
            throw std::logic_error("Not implemented yet.");
    }
    str << ", instruction: ";
    if (_instruction == nullptr) {
        str << "--";
    } else {
        str << *_instruction;
    }
    str << "}";
    return os << str.str();
}

void Vertex::setInstruction(std::unique_ptr<Instruction> instruction) {
    _instruction = std::move(instruction);
}

unsigned int Vertex::getLabel() const {
    return _label;
}

Instruction *Vertex::getInstruction() const {
    return _instruction.get();
}

Vertex::Type Vertex::getType() const {
    return _type;
}

void Vertex::setType(Type type) {
    _type = type;
}