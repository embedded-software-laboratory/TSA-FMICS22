#ifndef AHORN_INTEGER_CONSTANT_H
#define AHORN_INTEGER_CONSTANT_H

#include "ir/expression/constant/constant.h"

namespace ir {
    class IntegerConstant : public Constant {
    public:
        explicit IntegerConstant(int value);
        int getValue() const;
        std::unique_ptr<IntegerConstant> clone() const {
            return std::unique_ptr<IntegerConstant>(static_cast<IntegerConstant *>(this->clone_implementation()));
        }
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;

    private:
        int _value;
        IntegerConstant *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_INTEGER_CONSTANT_H
