#ifndef AHORN_DERIVED_TYPE_H
#define AHORN_DERIVED_TYPE_H

#include "ir/type/data_type.h"

namespace ir {
    class DerivedType : public DataType {
    public:
        explicit DerivedType(std::string name);

        std::unique_ptr<DerivedType> clone() const {
            return std::unique_ptr<DerivedType>(static_cast<DerivedType *>(this->clone_implementation()));
        }

        std::ostream &print(std::ostream &os) const override;

        bool operator==(const DerivedType &other) const {
            return getName() == other.getName();
        }

    private:
        DerivedType *clone_implementation() const override;

    protected:
        bool equals(const DataType &other) const override {
            const auto *derived_type = dynamic_cast<const DerivedType *>(&other);
            return derived_type != nullptr && static_cast<const DerivedType &>(*this) == *derived_type;
        }
    };
}// namespace ir

#endif//AHORN_DERIVED_TYPE_H
