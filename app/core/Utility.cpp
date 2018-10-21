#include "Utility.hpp"

static constexpr const char* logPattern = "%T.%e %n %t [%l] %^%v%$";

std::shared_ptr<spdlog::logger> getOrCreateLogger(const char* name) {
    if (spdlog::get(name)) {
        return spdlog::get(name);
    }

    auto log = spdlog::stdout_color_mt(name);
    log->set_pattern(logPattern);
    return log;
}
