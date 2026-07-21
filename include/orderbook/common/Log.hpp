#include <chrono>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#define LM_LIMIT_LOG(level, duration, ...)                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        thread_local std::chrono::time_point<std::chrono::steady_clock> lastLogTime = {};                              \
        thread_local size_t count = 0;                                                                                 \
                                                                                                                       \
        ++count;                                                                                                       \
        if (std::chrono::steady_clock::now() - lastLogTime > duration)                                                 \
        {                                                                                                              \
            auto message = fmt::format(__VA_ARGS__) + fmt::format(" [{} times]", count);                               \
            SPDLOG_##level(message);                                                                                   \
            count = 0;                                                                                                 \
            lastLogTime = std::chrono::steady_clock::now();                                                            \
        }                                                                                                              \
    }                                                                                                                  \
    while (false);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LM_LIMIT_LOG_DEBUG(...) LM_LIMIT_LOG(DEBUG, __VA_ARGS__)
#else
#define LM_LIMIT_LOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LM_LIMIT_LOG_INFO(...) LM_LIMIT_LOG(INFO, __VA_ARGS__)
#else
#define LM_LIMIT_LOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LM_LIMIT_LOG_WARN(...) LM_LIMIT_LOG(WARN, __VA_ARGS__)
#else
#define LM_LIMIT_LOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LM_LIMIT_LOG_ERROR(...) LM_LIMIT_LOG(ERROR, __VA_ARGS__)
#else
#define LM_LIMIT_LOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LM_LIMIT_LOG_CRITICAL(...) LM_LIMIT_LOG(CRITICAL, __VA_ARGS__)
#else
#define LM_LIMIT_LOG_CRITICAL(...) (void)0
#endif
