#ifndef AHORN_TIME_CONSTANT_H
#define AHORN_TIME_CONSTANT_H

#include "ir/expression/constant/constant.h"

namespace ir {
    class TimeConstant : public Constant {
    public:
        explicit TimeConstant(int value);
        int getValue() const;
        std::unique_ptr<TimeConstant> clone() const {
            return std::unique_ptr<TimeConstant>(static_cast<TimeConstant *>(this->clone_implementation()));
        }
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;

    private:
        int _value;
        TimeConstant *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_TIME_CONSTANT_H
