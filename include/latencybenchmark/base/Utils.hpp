#pragma once

#include <algorithm>
#include <cmath>
#include <pthread.h>
#include <thread>
#include <vector>

namespace lm
{

inline int pinToCpuCore(std::jthread& t, int coreId)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    return pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
#else
    return 0;
#endif
}

inline int pinCurrentToCpuCore(int coreId)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
#else
    return 0;
#endif
}

template <typename T> auto calcPercentile(std::vector<T>& values, unsigned percentile)
{
    auto pos = static_cast<int>(std::ceil(percentile * values.size() / 100.0f) - 1);
    pos = std::clamp(pos, int(0), int(values.size() - 1));

    auto index = values.begin() + pos;

    std::nth_element(values.begin(), index, values.end());
    return *index;
}

} // namespace lm
