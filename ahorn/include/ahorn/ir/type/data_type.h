#ifndef AHORN_DATA_TYPE_H
#define AHORN_DATA_TYPE_H

#include "ir/name.h"

#include <iostream>

namespace ir {
    class DataType : public Name {
    public:
        ~DataType() override = default;

        enum class Kind { ELEMENTARY_TYPE, SAFETY_TYPE, DERIVED_TYPE, ENUMERATED_TYPE, INCONCLUSIVE_TYPE, SIMPLE_TYPE };

        friend bool operator==(const DataType &data_type1, const DataType &data_type2) {
            return data_type1.equals(data_type2);
        }

        std::unique_ptr<DataType> clone() const {
            return std::unique_ptr<DataType>(this->clone_implementation());
        }
        DataType::Kind getKind() const;

        virtual std::ostream &print(std::ostream &os) const = 0;
        friend std::ostream &operator<<(std::ostream &os, const DataType &data_type) {
            return data_type.print(os);
        }

    protected:
        DataType(std::string name, Kind kind);

        virtual bool equals(const DataType &other) const = 0;

    private:
        virtual DataType *clone_implementation() const = 0;

    private:
        DataType::Kind _kind;
    };
}// namespace ir

#endif//AHORN_DATA_TYPE_H
