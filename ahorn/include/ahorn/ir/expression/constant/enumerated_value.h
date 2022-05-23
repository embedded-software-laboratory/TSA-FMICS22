#ifndef AHORN_ENUMERATED_VALUE_H
#define AHORN_ENUMERATED_VALUE_H

#include "ir/expression/constant/constant.h"

namespace ir {
    class EnumeratedValue : public Constant {
    public:
        explicit EnumeratedValue(std::string value);
        std::string getValue() const;
        std::unique_ptr<EnumeratedValue> clone() const {
            return std::unique_ptr<EnumeratedValue>(static_cast<EnumeratedValue *>(this->clone_implementation()));
        }
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;

        void setIndex(int index);
        int getIndex() const;

    private:
        std::string _value;
        int _index;
        EnumeratedValue *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_ENUMERATED_VALUE_H
