#ifndef AHORN_INCONCLUSIVE_TYPE_H
#define AHORN_INCONCLUSIVE_TYPE_H

#include "ir/type/data_type.h"

namespace ir {
    /*
     * Serves as a placeholder type for parsing. A type could be used before it is defined, hence a placeholder is used
     * and replaced afterwards, when all types have been parsed.
     */
    class InconclusiveType : public DataType {
    public:
        explicit InconclusiveType(std::string name);

        std::unique_ptr<InconclusiveType> clone() const {
            return std::unique_ptr<InconclusiveType>(static_cast<InconclusiveType *>(this->clone_implementation()));
        }

        std::ostream &print(std::ostream &os) const override;

        bool operator==(const InconclusiveType &other) const {
            return getName() == other.getName();
        }

    private:
        InconclusiveType *clone_implementation() const override;

    protected:
        bool equals(const DataType &other) const override {
            const auto *inconclusive_type = dynamic_cast<const InconclusiveType *>(&other);
            return inconclusive_type != nullptr && static_cast<const InconclusiveType &>(*this) == *inconclusive_type;
        }
    };
}// namespace ir


#endif//AHORN_INCONCLUSIVE_TYPE_H
