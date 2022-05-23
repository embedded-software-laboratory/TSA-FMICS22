#ifndef AHORN_CFG_EDGE_H
#define AHORN_CFG_EDGE_H

#include "boost/optional/optional.hpp"

// an interprocedural edge is either a call or a return edge
// in case the interprocedural edge is a return edge, label denotes the label of the call within the caller
class Edge {
public:
    enum class Type {
        INTRAPROCEDURAL = 0,
        INTRAPROCEDURAL_CALL_TO_RETURN = 1,
        INTERPROCEDURAL_CALL = 2,
        INTERPROCEDURAL_RETURN = 3,
        TRUE_BRANCH = 4,
        FALSE_BRANCH = 5
    };

    Edge(unsigned int source_label, unsigned int target_label, Type type = Type::INTRAPROCEDURAL);
    Edge(unsigned int source_label, unsigned int target_label, std::string caller_name, unsigned int call_label,
         Type type);

    unsigned int getSourceLabel() const;
    void setSourceLabel(unsigned int source_label);
    unsigned int getTargetLabel() const;
    void setTargetLabel(unsigned int target_label);

    std::string getCallerName() const;
    // returns the label of the vertex where the corresponding call has occurred
    unsigned int getCallLabel() const;

    Type getType() const;

private:
    unsigned int _source_label;
    unsigned int _target_label;
    boost::optional<std::string> _caller_name;
    boost::optional<unsigned int> _call_label;
    Type _type;
};


#endif//AHORN_CFG_EDGE_H
