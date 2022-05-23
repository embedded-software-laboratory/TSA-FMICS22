#ifndef AHORN_FMT_FORMATTER_H
#define AHORN_FMT_FORMATTER_H

#include "cfg/cfg.h"

#include "z3++.h"

#include "spdlog/fmt/fmt.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <memory>

template<>
struct fmt::formatter<z3::expr> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const z3::expr &e, FormatContext &ctx) {
        return format_to(ctx.out(), "{}", e.to_string());
    }
};

template<>
struct fmt::formatter<std::vector<z3::expr>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::vector<z3::expr> &v, FormatContext &ctx) {
        std::stringstream str;
        str << "[";
        for (auto it = v.begin(); it != v.end(); ++it) {
            str << *it;
            if (std::next(it) != v.end()) {
                str << ", ";
            }
        }
        str << "]";
        return format_to(ctx.out(), "{}", str.str());
    }
};


template<>
struct fmt::formatter<std::map<std::string, z3::expr>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::map<std::string, z3::expr> &m, FormatContext &ctx) {
        std::stringstream str;
        str << "{";
        for (auto it = m.begin(); it != m.end(); ++it) {
            str << it->first << " -> " << it->second;
            if (std::next(it) != m.end()) {
                str << ", ";
            }
        }
        str << "}";
        return format_to(ctx.out(), "{}", str.str());
    }
};

template<>
struct fmt::formatter<std::map<unsigned int, std::map<std::string, z3::expr>>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::map<unsigned int, std::map<std::string, z3::expr>> &m, FormatContext &ctx) {
        std::stringstream str;
        str << "{\n";
        for (auto it = m.begin(); it != m.end(); ++it) {
            str << "\tcycle " << it->first << ": [";
            for (auto valuation = it->second.begin(); valuation != it->second.end(); ++valuation) {
                str << valuation->first << " -> " << valuation->second;
                if (std::next(valuation) != it->second.end()) {
                    str << ", ";
                }
            }
            str << "]";
            if (std::next(it) != m.end()) {
                str << ",\n";
            }
        }
        str << "\n}";
        return format_to(ctx.out(), "{}", str.str());
    }
};

template<>
struct fmt::formatter<std::set<unsigned int>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::set<unsigned int> &s, FormatContext &ctx) {
        std::stringstream str;
        str << "{";
        for (auto it = s.begin(); it != s.end(); ++it) {
            str << *it;
            if (std::next(it) != s.end()) {
                str << ", ";
            }
        }
        str << "}";
        return format_to(ctx.out(), "{}", str.str());
    }
};

template<>
struct fmt::formatter<std::vector<unsigned int>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::vector<unsigned int> &v, FormatContext &ctx) {
        std::stringstream str;
        str << "[";
        for (auto it = v.begin(); it != v.end(); ++it) {
            str << *it;
            if (std::next(it) != v.end()) {
                str << ", ";
            }
        }
        str << "]";
        return format_to(ctx.out(), "{}", str.str());
    }
};

template<>
struct fmt::formatter<std::map<unsigned int, std::vector<unsigned int>>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::map<unsigned int, std::vector<unsigned int>> &m, FormatContext &ctx) {
        std::stringstream str;
        str << "{\n";
        for (auto it = m.begin(); it != m.end(); ++it) {
            str << "\tcycle " << it->first << ": [";
            for (auto label_it = it->second.begin(); label_it != it->second.end(); ++label_it) {
                str << *label_it;
                if (std::next(label_it) != it->second.end()) {
                    str << ", ";
                }
            }
            str << "]";
            if (std::next(it) != m.end()) {
                str << ",\n";
            }
        }
        str << "\n}";
        return format_to(ctx.out(), "{}", str.str());
    }
};


template<>
struct fmt::formatter<std::map<
        unsigned int,
        std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>, std::vector<unsigned int>>>>> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto
    format(const std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                              std::vector<unsigned int>>>> &m,
           FormatContext &ctx) {
        std::stringstream str;
        str << "{\n";
        for (auto it1 = m.begin(); it1 != m.end(); ++it1) {
            str << "\tcycle " << it1->first << ": [\n";
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
                str << "\t\t(\"" << it2->first.first << "\", " << (it2->first.second.get().getName()) << "): [";
                for (auto it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                    str << (*it3);
                    if (std::next(it3) != it2->second.end()) {
                        str << ", ";
                    }
                }
                str << "]";
                if (std::next(it2) != it1->second.end()) {
                    str << ",\n";
                }
            }
            str << "\n\t]";
            if (std::next(it1) != m.end()) {
                str << ",\n";
            }
        }
        str << "\n}";
        return format_to(ctx.out(), "{}", str.str());
    }
};

#endif//AHORN_FMT_FORMATTER_H
