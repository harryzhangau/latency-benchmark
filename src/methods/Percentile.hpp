#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <stdint.h>

#include "common/Utils.hpp"
#include "instrument/Benchmark.hpp"

namespace lm
{
template <typename T> class Percentile : public Benchmark<Percentile<T>, T>
{
public:
    using Benchmark<Percentile<T>, T>::Benchmark;

    void beforeOrderProcessing(const T& obj)
    {
        startTime = std::chrono::steady_clock::now();
    }

    void afterOrderProcessing(const T& obj)
    {
        auto finish_time = std::chrono::steady_clock::now();

        processingTimeNs[counter] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time - startTime).count();

        counter++;
    }

    bool isFinished()
    {
        return counter >= MAX_TRIES;
    }

    void complete()
    {
        SPDLOG_INFO("processed time 99th percentile: {} ns", calcPercentile(processingTimeNs, 99));
        SPDLOG_INFO("processed time 90th percentile: {} ns", calcPercentile(processingTimeNs, 90));
        SPDLOG_INFO("processed time 50th percentile: {} ns", calcPercentile(processingTimeNs, 50));
        SPDLOG_INFO("processed time 30th percentile: {} ns", calcPercentile(processingTimeNs, 30));
    }

private:
    static constexpr size_t MAX_TRIES = 10'000;

    std::chrono::steady_clock::time_point startTime;
    std::vector<uint64_t> processingTimeNs = std::vector<uint64_t>(MAX_TRIES);
    size_t counter = 0;
};

} // namespace lm