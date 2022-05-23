#include "se/cbmc/context/frame.h"

#include <sstream>

using namespace se::cbmc;

Frame::Frame(const Cfg &cfg, std::string scope, unsigned int return_label)
    : _cfg(cfg), _scope(std::move(scope)), _return_label(return_label) {}

std::ostream &Frame::print(std::ostream &os) const {
    std::stringstream str;
    str << "<" << _cfg.getName() << ", "
        << "\"" << _scope << "\""
        << ", " << _return_label << ">";
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