#pragma once

#include <stdint.h>
#include <variant>

namespace lm
{
struct AddOrder
{
    enum class Side
    {
        BUY,
        SELL
    };

    uint32_t id;
    Side side;
    uint32_t price;
    uint32_t quantity;
};

struct DeleteOrder
{
    uint32_t id;
};

struct ReplaceOrder
{
    uint32_t originalId;
    uint32_t newId;
    uint32_t price;
    uint32_t quantity;
};

using OrderMessage = std::variant<AddOrder, DeleteOrder, ReplaceOrder>;

} // namespace lm
