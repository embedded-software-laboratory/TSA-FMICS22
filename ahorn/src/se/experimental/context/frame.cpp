#include "se/experimental/context/frame.h"

#include <sstream>

using namespace se;

Frame::Frame(const Cfg &cfg, std::string scope, int label)
    : _cfg(cfg), _scope(std::move(scope)), _label(label),
      _local_path_constraint(std::vector<std::shared_ptr<Expression>>()) {}

std::ostream &Frame::print(std::ostream &os) const {
    std::stringstream str;
    str << "<";
    str << _cfg.getName() << ", \"" << _scope << "\", " << _label;
    str << ", ";
    str << "[";
    for (auto it = _local_path_constraint.begin(); it != _local_path_constraint.end(); ++it) {
        str << **it;
        if (std::next(it) != _local_path_constraint.end()) {
            str << ", ";
        }
    }
    str << "]";
    str << ">";
    return os << str.str();
}

const Cfg &Frame::getCfg() const {
    return _cfg;
}

std::string Frame::getScope() const {
    return _scope;
}

int Frame::getLabel() const {
    return _label;
}

const std::vector<std::shared_ptr<Expression>> &Frame::getLocalPathConstraint() const {
    return _local_path_constraint;
}

void Frame::pushLocalPathConstraint(std::shared_ptr<Expression> expression) {
    _local_path_constraint.push_back(std::move(expression));
}

void Frame::popLocalPathConstraints() {
    _local_path_constraint.clear();
}