#include "ir/variable.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

using namespace ir;

Variable::Variable(std::string name, std::unique_ptr<DataType> data_type, StorageType storage_type,
                   std::unique_ptr<Constant> initialization)
    : Name(std::move(name)), _data_type(std::move(data_type)), _storage_type(storage_type),
      _initialization(std::move(initialization)) {}

const DataType &Variable::getDataType() const {
    return *_data_type;
}

Variable::StorageType Variable::getStorageType() const {
    return _storage_type;
}

void Variable::setStorageType(StorageType storage_type) {
    _storage_type = storage_type;
}

void Variable::setType(std::unique_ptr<DataType> type) {
    _data_type = std::move(type);
}

std::ostream &Variable::print(std::ostream &os) const {
    std::stringstream str;
    str << getName() << " : " << *_data_type;
    return os << str.str();
}

bool Variable::hasInitialization() const {
    return _initialization != nullptr;
}

const Constant &Variable::getInitialization() const {
    assert(hasInitialization());
    return *_initialization;
}

namespace ir {
    bool operator==(const Variable &variable1, const Variable &variable2) {
        return (variable1.getFullyQualifiedName() == variable2.getFullyQualifiedName()) &&
               (variable1._data_type == variable2._data_type) && (variable1._storage_type == variable2._storage_type);
    }

    bool operator!=(const Variable &v1, const Variable &v2) {
        return !(v1 == v2);
    }
}// namespace ir
