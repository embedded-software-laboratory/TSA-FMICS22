#ifndef AHORN_BASIC_BLOCK_PASS_H
#define AHORN_BASIC_BLOCK_PASS_H

#include "cfg/cfg.h"

namespace pass {
    class BasicBlockPass {
    public:
        BasicBlockPass() = default;

        std::shared_ptr<Cfg> apply(const Cfg &cfg);
    };
}// namespace pass

#endif//AHORN_BASIC_BLOCK_PASS_H
