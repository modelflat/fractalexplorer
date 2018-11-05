#ifndef FRACTALEXPLORER_UTILITY_HPP
#define FRACTALEXPLORER_UTILITY_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/format.h>
#include <string>
#include <sstream>

#include <absl/strings/str_join.h>
#include <absl/strings/strip.h>

std::shared_ptr<spdlog::logger> getOrCreateLogger(const char *name);

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define LOGGER() static const auto logger = getOrCreateLogger(__FILE__ + SOURCE_PATH_SIZE);

#define LOC "[" __FUNCTION__ ":" STRINGIFY(__LINE__) "]"

#endif //FRACTALEXPLORER_UTILITY_HPP
