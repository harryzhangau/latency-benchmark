#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <stdint.h>

#include "instrument/Benchmark.hpp"

namespace lm
{
template <typename T> class Throughput : public Benchmark<Throughput<T>, T>
{
public:
    using Benchmark<Throughput<T>, T>::Benchmark;

    void beforeOrderProcessing(const T& obj)
    {
    }

    void afterOrderProcessing(const T& obj)
    {
        if (++counter % REPORT_INTERVAL == 0)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();

            SPDLOG_INFO("processed {} orders, {} orders/second, sitting orders: {}", counter,
                        REPORT_INTERVAL * 1000ULL / elapsed, obj.size());

            lastTime = now;
        }
    }

    bool isFinished()
    {
        return counter >= 20'000'000;
    }

    void complete()
    {
        // no op
    }

private:
    size_t counter = 0;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

    static constexpr size_t REPORT_INTERVAL = 1'000'000ULL;
};

} // namespace lm