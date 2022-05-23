#ifndef AHORN_OSTREAM_OPERATORS_H
#define AHORN_OSTREAM_OPERATORS_H

#include "z3++.h"

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

// KEEP THIS NAMESPACE WRAP: https://github.com/gabime/spdlog/issues/1917
// XXX pretty-printing
namespace std {
    template<typename T1, typename T2>
    std::ostream &operator<<(std::ostream &os, const std::map<T1, std::vector<std::shared_ptr<T2>>> &m) {
        std::stringstream str;
        str << "{";
        for (typename std::map<T1, std::vector<std::shared_ptr<T2>>>::const_iterator it = m.begin(); it != m.end();
             ++it) {
            str << it->first << " -> " << it->second;
            if (std::next(it) != m.end()) {
                str << ", ";
            }
        }
        str << "}";
        return os << str.str();
    }

    template<typename T1, typename T2>
    std::ostream &operator<<(std::ostream &os, const std::map<T1, std::shared_ptr<T2>> &m) {
        std::stringstream str;
        str << "{";
        for (typename std::map<T1, std::shared_ptr<T2>>::const_iterator it = m.begin(); it != m.end(); ++it) {
            str << it->first << " -> " << *(it->second);
            if (std::next(it) != m.end()) {
                str << ", ";
            }
        }
        str << "}";
        return os << str.str();
    }

    template<typename T>
    std::ostream &operator<<(std::ostream &os, const std::vector<std::shared_ptr<T>> &v) {
        std::stringstream str;
        str << "[";
        for (typename std::vector<std::shared_ptr<T>>::const_iterator it = v.begin(); it != v.end(); ++it) {
            str << **it;
            if (std::next(it) != v.end()) {
                str << ", ";
            }
        }
        str << "]";
        return os << str.str();
    }

    template<typename T>
    std::ostream &operator<<(std::ostream &os, const std::set<T> &s) {
        std::stringstream str;
        str << "(";
        for (typename std::set<T>::const_iterator it = s.begin(); it != s.end(); ++it) {
            str << *it;
            if (std::next(it) != s.end()) {
                str << ", ";
            }
        }
        str << ")";
        return os << str.str();
    }

    template<typename T>
    std::ostream &operator<<(std::ostream &os, const std::deque<std::shared_ptr<T>> &d) {
        std::stringstream str;
        str << "[";
        for (typename std::deque<std::shared_ptr<T>>::const_iterator it = d.begin(); it != d.end(); ++it) {
            str << **it;
            if (std::next(it) != d.end()) {
                str << ", ";
            }
        }
        str << "]";
        return os << str.str();
    }
}// namespace std

#endif//AHORN_OSTREAM_OPERATORS_H
