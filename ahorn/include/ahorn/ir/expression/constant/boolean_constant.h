#ifndef AHORN_BOOLEAN_CONSTANT_H
#define AHORN_BOOLEAN_CONSTANT_H

#include "ir/expression/constant/constant.h"

namespace ir {
    class BooleanConstant : public Constant {
    public:
        explicit BooleanConstant(bool value);
        bool getValue() const;
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<BooleanConstant> clone() const {
            return std::unique_ptr<BooleanConstant>(static_cast<BooleanConstant *>(this->clone_implementation()));
        }

    private:
        BooleanConstant *clone_implementation() const override;
        bool _value;
    };
}// namespace ir

#endif//AHORN_BOOLEAN_CONSTANT_H
