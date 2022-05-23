#ifndef AHORN_SMART_EXPLORER_H
#define AHORN_SMART_EXPLORER_H

#include "se/smart/context/context.h"

#include <memory>
#include <vector>

namespace se::smart {
    class Explorer {
    public:
        Explorer();

        bool isEmpty() const;

        std::unique_ptr<Context> popContext();

        void pushContext(std::unique_ptr<Context> context);

    private:
        std::vector<std::unique_ptr<Context>> _contexts;
    };
}

#endif//AHORN_SMART_EXPLORER_H
