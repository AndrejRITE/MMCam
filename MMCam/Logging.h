#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

inline void InitLogging()
{
    static bool inited = false;
    if (inited) return;

    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("MMCam.log", /*5 MB*/5 * 1024 * 1024, /*files*/3);
    auto logger = std::make_shared<spdlog::logger>("MMCam", sink);

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tid %t] %v");

    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);
    inited = true;
}

