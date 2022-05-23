#include "ir/type/elementary_type.h"

using namespace ir;

ElementaryType::ElementaryType(std::string name) : DataType(std::move(name), DataType::Kind::ELEMENTARY_TYPE) {
    if (getName() == "INT") {
        _type = Type::INT;
    } else if (getName() == "REAL") {
        _type = Type::REAL;
    } else if (getName() == "BOOL") {
        _type = Type::BOOL;
    } else if (getName() == "TIME") {
        // XXX TIME is modeled as integer with resolution of 1 ms
        _type = Type::TIME;
    } else if (getName() == "WORD") {
        // XXX WORD is modeled as integer, this might be incorrect...
        _type = Type::INT;
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

ElementaryType *ElementaryType::clone_implementation() const {
    return new ElementaryType(getName());
}

ElementaryType::Type ElementaryType::getType() const {
    return _type;
}

std::ostream &ElementaryType::print(std::ostream &os) const {
    switch (_type) {
        case Type::INT:
            return os << "INT";
        case Type::REAL:
            return os << "REAL";
        case Type::BOOL:
            return os << "BOOL";
        default:
            throw std::logic_error("Not implemented yet.");
    }
}
