#ifndef FRACTALEXPLORER_UTILITY_HPP
#define FRACTALEXPLORER_UTILITY_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/format.h>
#include <string>
#include <sstream>

template<typename Iter>
std::string join(Iter begin_, Iter end_, const char *sep = " ") {
    std::ostringstream ss;
    ss << *begin_;
    begin_++;
    for (; begin_ != end_; begin_++) {
        ss << sep << *begin_;
    }
    return ss.str();
}

std::shared_ptr<spdlog::logger> getOrCreateLogger(const char *name);

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define LOGGER() static const auto logger = getOrCreateLogger(__FILE__ + SOURCE_PATH_SIZE);

#define LOC "[" __FUNCTION__ ":" STRINGIFY(__LINE__) "]"

#endif //FRACTALEXPLORER_UTILITY_HPP
