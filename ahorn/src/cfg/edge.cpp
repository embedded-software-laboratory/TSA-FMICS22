#include "cfg/edge.h"

Edge::Edge(unsigned int source_label, unsigned int target_label, Edge::Type type)
    : _source_label(source_label), _target_label(target_label), _caller_name(boost::optional<std::string>()),
      _call_label(boost::optional<int>()), _type(type) {}

Edge::Edge(unsigned int source_label, unsigned int target_label, std::string caller_name, unsigned int call_label,
           Edge::Type type)
    : _source_label(source_label), _target_label(target_label), _caller_name(std::move(caller_name)),
      _call_label(call_label), _type(type) {
    assert(_type == Type::INTERPROCEDURAL_RETURN);
}

unsigned int Edge::getSourceLabel() const {
    return _source_label;
}

void Edge::setSourceLabel(unsigned int source_label) {
    _source_label = source_label;
}

unsigned int Edge::getTargetLabel() const {
    return _target_label;
}

void Edge::setTargetLabel(unsigned int target_label) {
    _target_label = target_label;
}

Edge::Type Edge::getType() const {
    return _type;
}

unsigned int Edge::getCallLabel() const {
    if (!_call_label.has_value()) {
        throw std::logic_error("Only valid for edges of type INTERPROCEDURAL_RETURN.");
    }
    return _call_label.get();
}

std::string Edge::getCallerName() const {
    if (!_caller_name.has_value()) {
        throw std::logic_error("Only valid for edges of type INTERPROCEDURAL_RETURN.");
    }
    return _caller_name.get();
}
