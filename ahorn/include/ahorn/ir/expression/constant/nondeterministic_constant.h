#ifndef AHORN_NONDETERMINISTIC_CONSTANT_H
#define AHORN_NONDETERMINISTIC_CONSTANT_H

#include "ir/expression/constant/constant.h"

namespace ir {
    class NondeterministicConstant : public Constant {
    public:
        explicit NondeterministicConstant(Expression::Type type);
        void accept(ExpressionVisitor &visitor) const override;
        void accept(NonConstExpressionVisitor &visitor) override;
        std::ostream &print(std::ostream &os) const override;
        std::unique_ptr<NondeterministicConstant> clone() const {
            return std::unique_ptr<NondeterministicConstant>(
                    static_cast<NondeterministicConstant *>(this->clone_implementation()));
        }

    private:
        NondeterministicConstant *clone_implementation() const override;
    };
}// namespace ir

#endif//AHORN_NONDETERMINISTIC_CONSTANT_H
