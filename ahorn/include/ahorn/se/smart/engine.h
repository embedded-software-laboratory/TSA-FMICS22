#ifndef AHORN_SMART_ENGINE_H
#define AHORN_SMART_ENGINE_H

#include <gtest/gtest_prod.h>

#include "cfg/cfg.h"
#include "se/smart/executor.h"
#include "se/smart/explorer.h"
#include "se/smart/z3/manager.h"

#include <memory>

class TestLibSeSmart_Call_Coverage_In_3_Cycles_Cfg_Test;

namespace se::smart {
    class Engine {
    private:
        FRIEND_TEST(::TestLibSeSmart, Call_Coverage_In_3_Cycles_Cfg);

    public:
        Engine();

        void run(const Cfg &cfg);

    private:
        void initialize(const Cfg &cfg);

        std::unique_ptr<Context> createInitialContext(const Cfg &cfg);

        std::vector<std::unique_ptr<Context>> step();

    private:
        unsigned int _cycle;
        std::unique_ptr<Manager> _manager;
        std::unique_ptr<Explorer> _explorer;
        std::unique_ptr<Executor> _executor;
    };
}// namespace se::smart

#endif//AHORN_SMART_ENGINE_H
