#pragma once

#include <stdint.h>
#include <variant>

namespace lm::itch
{

struct alignas(8) AddOrderMessage
{
    enum class Side
    {
        BUY,
        SELL
    };

    uint64_t orderId;
    Side side;
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp;
};

struct alignas(8) OrderExecutedMessage
{
    uint64_t orderId;
    uint32_t quantity;
    uint64_t timestamp;
};
struct alignas(64) OrderExecutedWithPriceMessage
{
    uint64_t orderId;
    uint32_t quantity;
    uint32_t price;
    uint64_t timestamp;
};

struct alignas(8) OrderCancelMessage
{
    uint64_t orderId;
    uint32_t quantity;
    uint64_t timestamp;
};

struct alignas(8) OrderDeleteMessage
{
    uint64_t orderId;
    uint64_t timestamp;
};

struct alignas(8) OrderReplaceMessage
{
    uint64_t originalOrderId;
    uint64_t newOrderId;
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp;
};

struct alignas(8) NonCrossTradeMessage
{
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp;
};

struct alignas(8) CrossTradeMessage
{
    enum class Direction
    {
        BUY,
        SELL,
        NO_IMBALANCE,
        INSUFFICIENT,
        PAUSED,
    };

    Direction imbalanceDirection;
    uint32_t price;
    uint32_t paireQuantity;
    uint32_t imbalanceQuantity;
    uint64_t timestamp;
};

using ItchMessage = std::variant<AddOrderMessage, OrderExecutedMessage, OrderExecutedWithPriceMessage,
                                 OrderCancelMessage, OrderReplaceMessage, NonCrossTradeMessage, CrossTradeMessage>;

} // namespace lm::itch