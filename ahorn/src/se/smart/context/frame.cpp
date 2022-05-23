#include "se/smart/context/frame.h"

#include <sstream>
#include <utility>

using namespace se::smart;

Frame::Frame(const Cfg &cfg, std::string scope, unsigned int return_label, int executed_conditionals)
    : _cfg(cfg), _scope(std::move(scope)), _return_label(return_label), _executed_conditionals(executed_conditionals) {}

std::ostream &Frame::print(std::ostream &os) const {
    std::stringstream str;
    str << "<" << _cfg.getName() << ", "
        << "\"" << _scope << "\""
        << ", " << _return_label << ", " << _executed_conditionals << ">";
    return os << str.str();
}

const Cfg &Frame::getCfg() const {
    return _cfg;
}

const std::string &Frame::getScope() const {
    return _scope;
}

unsigned int Frame::getReturnLabel() const {
    return _return_label;
}

int Frame::getExecutedConditionals() const {
    return _executed_conditionals;
}