#pragma once

#include <algorithm>
#include <cmath>
#include <pthread.h>
#include <thread>

namespace lm
{
// pos must be between 0 to 63 inclusive
inline constexpr uint64_t higherBitsMask(unsigned pos) noexcept
{
    if (pos >= 63)
        return 0;

    return ~(0ULL) << ((pos & 0x3f) + 1);
}

// pos must be between 0 to 63 inclusive
inline constexpr uint64_t lowerBitsMask(unsigned pos) noexcept
{
    if (pos == 0 || pos > 63)
        return 0;

    return ~(0ULL) >> (64 - (pos & 0x3f));
}

template <size_t N> inline void setBitmap(std::array<uint64_t, N>& bitmap, uint32_t price) noexcept
{
    bitmap[price >> 6] |= (1ULL << (price & 0x3f));
}

template <size_t N> inline void clearBitmap(std::array<uint64_t, N>& bitmap, uint32_t price) noexcept
{
    bitmap[price >> 6] &= ~(1ULL << (price & 0x3f));
}

} // namespace lm
