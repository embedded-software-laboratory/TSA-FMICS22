#ifndef AHORN_VARIABLE_H
#define AHORN_VARIABLE_H

#include "ir/expression/constant/constant.h"
#include "ir/name.h"
#include "ir/type/data_type.h"

#include <memory>

namespace ir {
    /**
     * Representation of a program variable referring to a memory location.
     */
    class Variable : public Name {
    public:
        enum class StorageType { INPUT, LOCAL, OUTPUT, TEMPORARY };

        Variable(std::string name, std::unique_ptr<DataType> data_type, StorageType storage_type,
                 std::unique_ptr<Constant> initialization = nullptr);

        // disable copy and copy assignment
        Variable(const Variable &other) = delete;
        Variable &operator=(const Variable &) = delete;

        friend bool operator==(const Variable &v1, const Variable &v2);
        friend bool operator!=(const Variable &v1, const Variable &v2);

        void setType(std::unique_ptr<DataType> type);
        const DataType &getDataType() const;

        StorageType getStorageType() const;
        void setStorageType(StorageType storage_type);

        bool hasInitialization() const;
        const Constant &getInitialization() const;

        friend std::ostream &operator<<(std::ostream &os, Variable const &variable) {
            return variable.print(os);
        }
        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, StorageType storage_type) {
            switch (storage_type) {
                case StorageType::INPUT:
                    return os << "INPUT";
                case StorageType::LOCAL:
                    return os << "LOCAL";
                case StorageType::OUTPUT:
                    return os << "OUTPUT";
                case StorageType::TEMPORARY:
                    return os << "TEMPORARY";
            }
        }

    private:
        std::unique_ptr<DataType> _data_type;
        StorageType _storage_type;
        std::unique_ptr<Constant> _initialization;
    };

    class VariableKeyHasher {
    public:
        std::size_t operator()(const Variable &v) const {
            using std::hash;
            using std::size_t;
            using std::string;

            size_t res = 17;
            res = res * 31 + hash<string>()(v.getFullyQualifiedName());
            return res;
        }
    };

    class VariableKeyComparator {
    public:
        bool operator()(const Variable &v1, const Variable &v2) const {
            return v1.getFullyQualifiedName() == v2.getFullyQualifiedName();
        }
    };
}// namespace ir

#endif//AHORN_VARIABLE_H
