#pragma once
#include <chrono>
#include <string_view>
#include <spdlog/spdlog.h>

struct ScopeTimer 
{
    std::string label;
    std::chrono::steady_clock::time_point t0;

    explicit ScopeTimer(std::string_view lbl)
        : label(lbl), t0(std::chrono::steady_clock::now()) { }

    ~ScopeTimer() 
    {
        using namespace std::chrono;
        const auto ms = duration_cast<microseconds>(steady_clock::now() - t0).count() / 1000.0;
        spdlog::info("{} took {:.3f} ms", label, ms);
    }
};

// Convenience macro (unique var per line)
#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)
#define SCOPE_TIMER(name) ScopeTimer CAT(__scope_timer_, __LINE__){name}

