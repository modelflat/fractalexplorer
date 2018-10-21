#ifndef FRACTALEXPLORER_CORELOGGER_HPP
#define FRACTALEXPLORER_CORELOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static constexpr const char* logPattern = "[%T.%e] [%t] [%l] [%n] : %v";

static auto logger = []() {
    auto log = spdlog::stdout_color_mt("Core");
    log->set_pattern(logPattern);
    return log;
}();

#endif //FRACTALEXPLORER_CORELOGGER_HPP
