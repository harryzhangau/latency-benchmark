#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>

#include "ExchangeOrders.hpp"
#include "OrderMessage.hpp"
#include "common/CallbackStorage.hpp"
#include "common/Log.hpp"
#include "common/ObjectPool.hpp"

class TestExchangeOrderBook;

namespace lm
{

template <class... Ts> struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <typename MessageBusType> class ExchangeOrderBook
{
public:
    static constexpr size_t POOL_SIZE = 5'000'000;

    struct LinkedExchangeOrderAllocator
    {
        static LinkedExchangeOrder* allocate()
        {
            return ExchangeOrderBook<MessageBusType>::pool.allocate();
        }
    };

    struct LinkedExchangeOrderDeleter : public ObjectPool<LinkedExchangeOrder, POOL_SIZE>::Deleter
    {
        LinkedExchangeOrderDeleter()
            : ObjectPool<LinkedExchangeOrder, POOL_SIZE>::Deleter(ExchangeOrderBook<MessageBusType>::pool)
        {
        }
    };

    ExchangeOrderBook(MessageBusType& messageBus)
        : messageBus(messageBus)
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
                    orderMap[inserted->id] = inserted;
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
                    orderMap[inserted->id] = inserted;
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
        auto it = orderMap.find(orderId);
        if (it == orderMap.end()) [[unlikely]]
            return;

        auto linkedOrder = it->second;
        linkedOrder->triggerCallback();

        orderMap.erase(orderId);
    }

    inline thread_local static ObjectPool<LinkedExchangeOrder, POOL_SIZE> pool;

    ExchangeOrders<BuySide, LinkedExchangeOrderAllocator, LinkedExchangeOrderDeleter> bids;
    ExchangeOrders<SellSide, LinkedExchangeOrderAllocator, LinkedExchangeOrderDeleter> asks;

    MessageBusType& messageBus;

    // @todo Replace it with low overhead map
    std::unordered_map<uint64_t, LinkedExchangeOrder*> orderMap;

    friend class ::TestExchangeOrderBook;
};

}; // namespace lm