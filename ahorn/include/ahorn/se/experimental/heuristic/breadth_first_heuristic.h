#ifndef AHORN_BREADTH_FIRST_HEURISTIC_H
#define AHORN_BREADTH_FIRST_HEURISTIC_H

#include "se/experimental/context/context.h"

#include <memory>

namespace se {
    class BreadthFirstHeuristic {
    public:
        // XXX priority queue returns the highest priority element, i.e., return true if the first argument
        // appears before the second in the strict weak ordering relation, hence in order to prioritize "lower"
        // cycles or "deeper" nested frames, the comparator is reversed!
        bool operator()(const std::unique_ptr<Context> &context_1, const std::unique_ptr<Context> &context_2) const {
            // prioritize lower cycles
            if (context_1->getCycle() > context_2->getCycle()) {
                return true;
            } else if (context_1->getCycle() == context_2->getCycle()) {
                // prioritize deeper nested contexts
                if (context_1->getFrameDepth() > context_2->getFrameDepth()) {
                    return false;
                } else if (context_1->getFrameDepth() == context_2->getFrameDepth()) {
                    const State &state_1 = context_1->getState();
                    const State &state_2 = context_2->getState();
                    // prioritize lower labels
                    const Vertex &vertex_1 = state_1.getVertex();
                    const Vertex &vertex_2 = state_2.getVertex();
                    if (vertex_1.getLabel() > vertex_2.getLabel()) {
                        return true;
                    } else if (vertex_1.getLabel() == vertex_2.getLabel()) {
                        // XXX execution contexts join after, e.g., returning from a callee
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return true;
                }
            } else {
                return false;
            }
        }
    };
}// namespace se

#endif//AHORN_BREADTH_FIRST_HEURISTIC_H
