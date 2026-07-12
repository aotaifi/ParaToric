// ParaToric - Continuous-time QMC for the extended toric code in the x/z-basis
// Copyright (C) 2022-2026  Simon Mathias Linsel, Lode Pollet

#pragma once

#include <iostream>
#include <string_view>

namespace paratoric::logging {

inline bool trivial_log_enabled(std::string_view level) {
    return level != "debug";
}

} // namespace paratoric::logging

#define BOOST_LOG_TRIVIAL(level) \
    if (!::paratoric::logging::trivial_log_enabled(#level)) {} else std::clog
