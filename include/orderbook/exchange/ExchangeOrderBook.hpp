#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>

#include <latencybenchmark/base/CallbackStorage.hpp>

#include "../common/HashMap.hpp"
#include "../common/Log.hpp"
#include "../common/ObjectPool.hpp"
#include "ExchangeOrders.hpp"
#include "OrderMessage.hpp"

class TestExchangeOrderBook;

namespace lm
{

template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};

class ExchangeOrderBook
{
public:
    static constexpr size_t MAP_SIZE = 1 << 23;
    static constexpr size_t POOL_SIZE = (MAP_SIZE * 3) / 4;

    struct LinkedExchangeOrderAllocator
    {
        static LinkedExchangeOrder* allocate()
        {
            return ExchangeOrderBook::pool.allocate();
        }
    };

    struct LinkedExchangeOrderDeleter : public ObjectPool<LinkedExchangeOrder, POOL_SIZE>::Deleter
    {
        LinkedExchangeOrderDeleter()
            : ObjectPool<LinkedExchangeOrder, POOL_SIZE>::Deleter(ExchangeOrderBook::pool)
        {
        }
    };

    ExchangeOrderBook()
    {
    }

    void processOrderMessage(const OrderMessage& orderMessage) noexcept
    {
        std::visit(overloaded{[this](const AddOrder& order) { addLimitOrder(order); },
                              [this](const DeleteOrder& order) { deleteOrder(order.id); },
                              [](const ReplaceOrder& order) {}},
                   orderMessage);
    }

    size_t size() const noexcept
    {
        return orderMap.size();
    }

private:
    void addLimitOrder(const AddOrder& order) noexcept
    {
        auto remaining = order.quantity;

        if (order.side == AddOrder::Side::BUY)
        {
            auto remaining = asks.match(order, [this](uint64_t orderId) {
                // Traded, remove it from order map
                orderMap.erase(orderId);
            });

            if (remaining > 0)
            {
                if (auto inserted = bids.insert({order.id, order.side, order.price, remaining})) [[likely]]
                {
                    orderMap.insert(inserted->id, inserted - pool.memory());
                }
                else [[unlikely]]
                {
                    LM_LIMIT_LOG_WARN(std::chrono::seconds{5}, "Failed to insert buy orders");
                }
            }
        }
        else
        {
            auto remaining = bids.match(order, [this](uint64_t orderId) {
                // Traded, remove it from order map
                orderMap.erase(orderId);
            });

            if (remaining > 0)
            {
                if (auto inserted = asks.insert({order.id, order.side, order.price, remaining})) [[likely]]
                {
                    orderMap.insert(inserted->id, inserted - pool.memory());
                }
                else [[unlikely]]
                {
                    LM_LIMIT_LOG_WARN(std::chrono::seconds{5}, "Failed to insert sell orders");
                }
            }
        }
    }

    void deleteOrder(uint64_t orderId) noexcept
    {
        auto linkedOrderOffset = orderMap.get(orderId);
        if (!linkedOrderOffset.has_value()) [[unlikely]]
            return;
        auto linkedOrder = pool.memory() + *linkedOrderOffset;

        linkedOrder->triggerCallback();

        orderMap.erase(orderId);
    }

    inline thread_local static ObjectPool<LinkedExchangeOrder, POOL_SIZE> pool;

    ExchangeOrders<BuySide, LinkedExchangeOrderAllocator, LinkedExchangeOrderDeleter> bids;
    ExchangeOrders<SellSide, LinkedExchangeOrderAllocator, LinkedExchangeOrderDeleter> asks;

    HashMap<uint64_t, uint64_t, std::countr_zero(MAP_SIZE)> orderMap;

    friend class ::TestExchangeOrderBook;
};

}; // namespace lm