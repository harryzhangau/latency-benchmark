#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>

#include <latencybenchmark/base/CallbackStorage.hpp>

#include "../common/Bitmap.hpp"
#include "../common/Log.hpp"
#include "ExchangePriceLevel.hpp"
#include "OrderMessage.hpp"

namespace lm
{

static constexpr size_t MAX_LEVELS = 1'000'00;
static constexpr size_t MAX_BITMAPS = (MAX_LEVELS / 64) + 1;

struct LinkedExchangeOrder : public CallbackStorage<void>
{
    uint32_t id;
    uint32_t price;
    uint32_t quantity;

    LinkedExchangeOrder* prev = nullptr;
    LinkedExchangeOrder* next = nullptr;
};

struct BuySide
{
    static constexpr inline bool betterOrSame(uint32_t ours, uint32_t theirs) noexcept
    {
        // For buyer sitting orders, higher price is better
        return ours >= theirs;
    }

    static constexpr inline int closestBetterPricePos(uint64_t word, unsigned pos) noexcept
    {
        uint64_t bits = word & higherBitsMask(pos);
        return std::countr_zero(bits);
    }

    static constexpr inline int closestBetterPricePos(uint64_t word) noexcept
    {
        return std::countr_zero(word);
    }

    static constexpr inline bool nextBetter(int& index, size_t max) noexcept
    {
        if (index < max) [[likely]]
        {
            ++index;
            return true;
        }

        return false;
    }
};

struct SellSide
{
    static constexpr inline bool betterOrSame(uint32_t ours, uint32_t theirs) noexcept
    {
        // For seller sitting orders, lower price is better
        return ours <= theirs;
    }

    static constexpr inline int closestBetterPricePos(uint64_t word, unsigned pos) noexcept
    {
        uint64_t bits = word & lowerBitsMask(pos);
        return 63 - std::countl_zero(bits);
    }

    static constexpr inline int closestBetterPricePos(uint64_t word) noexcept
    {
        return 63 - std::countl_zero(word);
    }

    static constexpr inline bool nextBetter(int& index, size_t max) noexcept
    {
        if (index > 0) [[likely]]
        {
            index--;
            return true;
        }

        return false;
    }
};

template <typename S, typename OrderAllocator, typename OrderDeleter> class ExchangeOrders
{
public:
    ExchangeOrders()
    {
        ::memset(&bitmap[0], 0, sizeof(bitmap));
    }

    template <typename ListenerType>
    uint32_t match(const AddOrder& order, const ListenerType& orderCompleteListener) noexcept
    {
        auto remaining = order.quantity;
        while (remaining > 0 && bestPrice != nullptr && S::betterOrSame(bestPrice->price, order.price))
        {
            assert(!bestPrice->empty());

            // auto originalCount = remaining;
            remaining = bestPrice->match(remaining, orderCompleteListener);

            if (bestPrice->empty())
            {
                clearBitmap(bitmap, bestPrice->price);
                bestPrice = bestPrice->next;
                if (bestPrice != nullptr)
                    bestPrice->prev = nullptr;

                assert(bestPrice == nullptr || !bestPrice->empty());
            }
        }

        return remaining;
    }

    LinkedExchangeOrder* insert(const AddOrder& order) noexcept
    {
        auto linked_order = OrderAllocator::allocate();
        if (linked_order == nullptr)
        {
            return nullptr;
        }

        auto& price_level = priceLevels[order.price];
        if (price_level.empty())
        {
            price_level.price = order.price;
            price_level.next = price_level.prev = nullptr;

            // bestPrice must not be empty
            assert(bestPrice != &price_level);

            if (bestPrice == nullptr)
            {
                bestPrice = &price_level;
            }
            else
            {
                assert(!bestPrice->empty());

                // It wouldn't be same, so it should either better or worse
                if (S::betterOrSame(order.price, bestPrice->price))
                {
                    price_level.next = bestPrice;
                    bestPrice->prev = &price_level;

                    bestPrice = &price_level;
                }
                else
                {
                    ExchangePriceLevel<LinkedExchangeOrder, OrderDeleter>* prev_price = nullptr;

                    // Search for next (lower or higher) price order
                    int index = order.price >> 6;
                    unsigned bit = S::closestBetterPricePos(bitmap[index], order.price & 0x3f);

                    do
                    {
                        if (bit >= 0 && bit < 64)
                        {
                            prev_price = &priceLevels[(index << 6) + bit];
                            break;
                        }

                        if (!S::nextBetter(index, MAX_LEVELS))
                            break;

                        uint64_t value = bitmap[index];
                        if (value == 0)
                            continue;

                        bit = S::closestBetterPricePos(bitmap[index]);
                    }
                    while (true);

                    if (prev_price != nullptr)
                    {
                        price_level.next = prev_price->next;
                        price_level.prev = prev_price;

                        if (prev_price->next != nullptr)
                        {
                            prev_price->next->prev = &price_level;
                        }

                        prev_price->next = &price_level;
                    }
                }
            }
        }

        linked_order->id = order.id;
        linked_order->quantity = order.quantity;
        linked_order->price = order.price;
        linked_order->next = linked_order->prev = nullptr;

        linked_order->setCallback(
            this, +[](void* delegate, CallbackStorage<void>* order) {
                static_cast<ExchangeOrders*>(delegate)->removeOrder(static_cast<LinkedExchangeOrder*>(order));
            });

        price_level.addOrder(linked_order);

        setBitmap(bitmap, price_level.price);

        assert(!price_level.empty());
        assert(!bestPrice->empty());

        return linked_order;
    }

    void removeOrder(LinkedExchangeOrder* order) noexcept
    {
        auto& price_level = priceLevels[order->price];
        price_level.removeOrder(order);

        if (price_level.empty())
        {
            clearBitmap(bitmap, price_level.price);

            if (&price_level == bestPrice)
            {
                bestPrice = price_level.next;
                if (bestPrice != nullptr)
                    bestPrice->prev = nullptr;
            }
            else
            {
                price_level.prev->next = price_level.next;

                if (price_level.next != nullptr)
                {
                    price_level.next->prev = price_level.prev;
                }
            }

            assert(bestPrice == nullptr || !bestPrice->empty());
        }
    }

private:
    std::array<ExchangePriceLevel<LinkedExchangeOrder, OrderDeleter>, MAX_LEVELS> priceLevels;
    std::array<uint64_t, MAX_BITMAPS> bitmap;
    ExchangePriceLevel<LinkedExchangeOrder, OrderDeleter>* bestPrice = nullptr;
};

} // namespace lm
