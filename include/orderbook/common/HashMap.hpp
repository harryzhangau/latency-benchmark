#pragma once

#include <stdint.h>

namespace lm
{

template <unsigned P> constexpr inline uint64_t hash(uint64_t key) noexcept
{
    static_assert(P > 0 && P < 64);

    static constexpr uint64_t MASK = (1 << P) - 1;
    return key & MASK;
}

template <typename K, typename V, unsigned P> class HashMap
{
public:
    bool insert(K key, V value) noexcept
    {
        assert(count < CAPACITY * 0.8);

        auto index = hash<P>(key);

        for (size_t i = 0; i < CAPACITY; i++)
        {
            auto pos = (index + i) & MASK;
            auto& slot = map[pos];

            if (!isOccupied(slot))
            {
                slot.key = key;
                setValueAndOccupied(slot, value);

                count++;
                return true;
            }

            if (slot.key == key)
            {
                slot.key = key;
                setValueAndOccupied(slot, value);
                return true;
            }
        }

        return false;
    }

    void erase(K key) noexcept
    {
        auto index = hash<P>(key);

        std::optional<size_t> deleted;
        for (size_t i = 0; i < CAPACITY; i++)
        {
            auto pos = (index + i) & MASK;
            auto& slot = map[pos];

            if (!isOccupied(slot))
            {
                return;
            }

            if (slot.key == key)
            {
                deleted = pos;
                break;
            }
        }

        if (deleted)
        {
            auto start = *deleted;
            auto vacancy = start;

            for (size_t i = 1; i < CAPACITY; i++)
            {
                auto pos = (start + i) & MASK;
                auto& slot = map[pos];

                if (!isOccupied(slot))
                {
                    break;
                }

                auto home = hash<P>(slot.key) & MASK;

                if (pos > vacancy && (home > vacancy && pos >= home) ||
                    pos < vacancy && (home > vacancy || home <= pos))
                {
                    continue;
                }

                map[vacancy] = slot;
                vacancy = pos;
            }

            clearOccupied(map[vacancy]);

            count--;
        }
    }

    bool contains(K key) const noexcept
    {
        auto index = hash<P>(key);

        for (size_t i = 0; i < CAPACITY; i++)
        {
            auto pos = (index + i) & MASK;
            auto& slot = map[pos];

            if (!isOccupied(slot))
            {
                return false;
            }

            if (slot.key == key)
            {
                return true;
            }
        }

        return false;
    }

    std::optional<V> get(K key) const noexcept
    {
        auto index = hash<P>(key);

        for (size_t i = 0; i < CAPACITY; i++)
        {
            auto pos = (index + i) & MASK;
            auto& slot = map[pos];

            if (!isOccupied(slot))
            {
                return std::nullopt;
            }

            if (slot.key == key)
            {
                return getValue(slot);
            }
        }

        return std::nullopt;
    }

    size_t size() const noexcept
    {
        return count;
    }

private:
    struct alignas(16) Slot
    {
        K key = 0;
        V value = {};
    };

    constexpr inline bool isOccupied(const Slot& slot) const
    {
        return slot.value & (1ULL << 63);
    }

    constexpr inline uint64_t getValue(const Slot& slot) const
    {
        return slot.value & ~(1ULL << 63);
    }

    constexpr inline void setValueAndOccupied(Slot& slot, uint64_t value)
    {
        slot.value = value | (1ULL << 63);
    }

    constexpr inline void setOccupied(Slot& slot)
    {
        slot.value |= (1ULL << 63);
    }

    constexpr inline void clearOccupied(Slot& slot)
    {
        slot.value &= ~(1ULL << 63);
    }

    static constexpr size_t CAPACITY = 1 << P;
    static constexpr uint64_t MASK = (1 << P) - 1;

    std::array<Slot, CAPACITY> map;

    size_t count = 0;
};

} // namespace lm