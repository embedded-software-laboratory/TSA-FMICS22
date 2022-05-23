#ifndef AHORN_CONFIGURATION_H
#define AHORN_CONFIGURATION_H

#include "boost/optional.hpp"

#include <map>
#include <vector>

namespace se {
    class Configuration {
    public:
        Configuration();

    public:
        boost::optional<std::string> _generate_test_suite;
        boost::optional<unsigned> _cycle_bound;
        boost::optional<long> _time_out;
        boost::optional<bool> _unreachable_information;
        boost::optional<std::vector<unsigned int>> _unreachable_labels;
        boost::optional<std::map<unsigned int, std::pair<bool, bool>>> _unreachable_branches;
    };
}// namespace se

#endif//AHORN_CONFIGURATION_H
