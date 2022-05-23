#ifndef AHORN_FRAME_H
#define AHORN_FRAME_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/experimental/expression/expression.h"

#include <memory>
#include <vector>

class TestLibSe_Frame_Test;

namespace se {
    class Frame {
    private:
        FRIEND_TEST(::TestLibSe, Frame);

    public:
        // XXX default constructor disabled
        Frame() = delete;
        // XXX copy constructor disabled
        Frame(const Frame &other) = delete;
        // XXX copy assignment disabled
        Frame &operator=(const Frame &) = delete;

        Frame(const Cfg &cfg, std::string scope, int label);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Frame &frame) {
            return frame.print(os);
        }

        const Cfg &getCfg() const;

        std::string getScope() const;

        int getLabel() const;

        const std::vector<std::shared_ptr<Expression>> &getLocalPathConstraint() const;

        void pushLocalPathConstraint(std::shared_ptr<Expression> expression);

        void popLocalPathConstraints();

    private:
        const Cfg &_cfg;

        std::string _scope;

        int _label;

        std::vector<std::shared_ptr<Expression>> _local_path_constraint;
    };
}// namespace se

#endif//AHORN_FRAME_H
