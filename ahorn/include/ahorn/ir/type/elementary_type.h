#ifndef AHORN_ELEMENTARY_TYPE_H
#define AHORN_ELEMENTARY_TYPE_H

#include "ir/type/data_type.h"

namespace ir {
    class ElementaryType : public DataType {
    public:
        typedef int INT_T;
        typedef double REAL_T;
        typedef bool BOOL_T;

        enum class Type { INT, REAL, BOOL, TIME };

        explicit ElementaryType(std::string name);

        ElementaryType::Type getType() const;

        std::ostream &print(std::ostream &os) const override;

        std::unique_ptr<ElementaryType> clone() const {
            return std::unique_ptr<ElementaryType>(static_cast<ElementaryType *>(this->clone_implementation()));
        }

        bool operator==(const ElementaryType &other) const {
            return _type == other._type;
        }

    private:
        ElementaryType *clone_implementation() const override;

    protected:
        bool equals(const DataType &other) const override {
            const auto *elementary_type = dynamic_cast<const ElementaryType *>(&other);
            return elementary_type != nullptr && static_cast<const ElementaryType &>(*this) == *elementary_type;
        }

    private:
        Type _type;
    };
}// namespace ir

#endif//AHORN_ELEMENTARY_TYPE_H
