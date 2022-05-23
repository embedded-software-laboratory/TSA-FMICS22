#ifndef AHORN_SIMPLE_TYPE_H
#define AHORN_SIMPLE_TYPE_H

#include "ir/type/data_type.h"

namespace ir {
    class SimpleType : public DataType {
    public:
        ~SimpleType() override = default;

        explicit SimpleType(std::string name);

        std::unique_ptr<SimpleType> clone() const {
            return std::unique_ptr<SimpleType>(static_cast<SimpleType *>(this->clone_implementation()));
        }

        std::ostream &print(std::ostream &os) const override;

        bool operator==(const SimpleType &other) const {
            return getName() == other.getName();
        }

    private:
        SimpleType *clone_implementation() const override;

    protected:
        bool equals(const DataType &other) const override {
            const auto *simple_type = dynamic_cast<const SimpleType *>(&other);
            return simple_type != nullptr && static_cast<const SimpleType &>(*this) == *simple_type;
        }
    };
}// namespace ir

#endif//AHORN_SIMPLE_TYPE_H
