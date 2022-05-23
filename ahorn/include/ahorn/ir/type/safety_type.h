#ifndef AHORN_SAFETY_TYPE_H
#define AHORN_SAFETY_TYPE_H

#include "ir/type/data_type.h"

namespace ir {
    class SafetyType : public DataType {
    public:
        enum class Kind { SAFEBOOL };

        explicit SafetyType(const std::string &name);

        std::unique_ptr<SafetyType> clone() const {
            return std::unique_ptr<SafetyType>(static_cast<SafetyType *>(this->clone_implementation()));
        }

        SafetyType::Kind getType() const;

        std::ostream &print(std::ostream &os) const override;

        bool operator==(const SafetyType &other) const {
            return _kind == other._kind;
        }

    private:
        SafetyType *clone_implementation() const override;

    protected:
        bool equals(const DataType &other) const override {
            const auto *safety_type = dynamic_cast<const SafetyType *>(&other);
            return safety_type != nullptr && static_cast<const SafetyType &>(*this) == *safety_type;
        }

    private:
        Kind _kind;
    };
}// namespace ir

#endif//AHORN_SAFETY_TYPE_H
