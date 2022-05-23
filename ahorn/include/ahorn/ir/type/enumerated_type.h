#ifndef AHORN_ENUMERATED_TYPE_H
#define AHORN_ENUMERATED_TYPE_H

#include "ir/type/data_type.h"

#include <map>

namespace ir {
    class EnumeratedType : public DataType {
    public:
        explicit EnumeratedType(std::string name);

        void addValue(std::string value);

        int getIndex(const std::string &value) const;

        std::unique_ptr<EnumeratedType> clone() const {
            return std::unique_ptr<EnumeratedType>(static_cast<EnumeratedType *>(this->clone_implementation()));
        }

        std::ostream &print(std::ostream &os) const override;

        bool operator==(const EnumeratedType &other) const {
            return getName() == other.getName();
        }
        const std::map<std::string, int> &getValueToIndex() const;

    protected:
        bool equals(const DataType &other) const override {
            const auto *enumerated_type = dynamic_cast<const EnumeratedType *>(&other);
            return enumerated_type != nullptr && static_cast<const EnumeratedType &>(*this) == *enumerated_type;
        }

    private:
        EnumeratedType *clone_implementation() const override;

    private:
        // internal index for an enum value
        int _index;
        std::map<std::string, int> _value_to_index;
    };
}// namespace ir

#endif//AHORN_ENUMERATED_TYPE_H
