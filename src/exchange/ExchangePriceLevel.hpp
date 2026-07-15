#pragma once

#include <list>
#include <memory>
#include <stdint.h>

#include "common/ObjectList.hpp"

namespace lm
{

template <typename T, typename Deleter = std::default_delete<T>> class ExchangePriceLevel
{
public:
    ExchangePriceLevel(Deleter deleter = Deleter())
        : orders(deleter)
    {
    }

    template <typename ListenerType> uint32_t match(uint32_t quantity, const ListenerType& listener) noexcept
    {
        if (quantity == 0)
            return 0;

        for (auto it = orders.front(); it != nullptr;)
        {
            if (it->quantity > quantity)
            {
                it->quantity -= quantity;
                return 0;
            }
            else
            {
                quantity -= it->quantity;

                auto orderId = it->id;
                it = orders.erase(it);

                // notify trade
                listener(orderId);
            }
        }

        return quantity;
    }

    void addOrder(T* order) noexcept
    {
        orders.push(order);
    }

    void removeOrder(T* order) noexcept
    {
        orders.erase(order);
    }

    bool empty() const noexcept
    {
        return orders.empty();
    }

    uint32_t price = 0;

    // Next lower or higher price
    ExchangePriceLevel* next = nullptr;
    ExchangePriceLevel* prev = nullptr;

private:
    ObjectList<T, Deleter> orders;
};
} // namespace lm