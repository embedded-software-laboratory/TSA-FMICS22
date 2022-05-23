#include "ir/type/data_type.h"

using namespace ir;

DataType::DataType(std::string name, Kind kind) : Name(std::move(name)), _kind(kind) {}

DataType::Kind DataType::getKind() const {
    return _kind;
}
