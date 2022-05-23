#include "ir/name.h"

using namespace ir;

Name::Name(std::string name) : _name(std::move(name)), _parent(nullptr) {}

std::string Name::getName() const {
    return _name;
}

void Name::setParent(const Name &parent) {
    _parent = &parent;
}

std::string Name::getFullyQualifiedName() const {
    if (_parent == nullptr) {
        return _name;
    } else {
        return _parent->getFullyQualifiedName() + "." + _name;
    }
}

const Name *Name::getParent() const {
    return _parent;
}
