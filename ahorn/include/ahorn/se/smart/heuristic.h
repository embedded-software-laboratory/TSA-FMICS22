#ifndef AHORN_SMART_HEURISTIC_H
#define AHORN_SMART_HEURISTIC_H

#include "se/smart/context/context.h"

namespace se::smart {
    class Heuristic {
    public:
        bool operator()(const std::unique_ptr<Context> &context_1, const std::unique_ptr<Context> &context_2) const {
            throw std::logic_error("Not implemented yet.");
        }
    };
}

#endif//AHORN_SMART_HEURISTIC_H
