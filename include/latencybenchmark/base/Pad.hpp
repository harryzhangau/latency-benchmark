#pragma once

#include <atomic>
#include <string>
#include <unordered_map>
#include <variant>

#include "CallbackStorage.hpp"

namespace lm
{

enum class PadType
{
    ACTIVE,
    PASSIVE
};

template <typename T> class Pad : public CallbackStorage<bool, T>
{
public:
    using U = std::decay_t<T>;
    using PadPtrVariant = std::variant<Pad<U>*, Pad<const U&>*, Pad<U&>*, Pad<U&&>*>;

    Pad(PadType type)
        : type(type)
    {
    }

    bool connect(PadPtrVariant pad) noexcept
    {
        auto* ptr = get_if<Pad<T>*>(&pad);
        if (!ptr)
        {
            return false;
        }

        if ((*ptr)->type == type)
        {
            return false;
        }

        this->connected = pad;
        return true;
    }

    void reset() noexcept
    {
        this->connected = {};
    }

    bool trigger(T data) noexcept
    {
        auto* ptr = get_if<Pad<T>*>(&connected);
        if (ptr && *ptr)
        {
            return (*ptr)->triggerCallback(data);
        }

        return false;
    }

private:
    PadPtrVariant connected;
    PadType type = PadType::PASSIVE;
};

template <typename T> bool connect(typename Pad<T>::PadPtrVariant src, typename Pad<T>::PadPtrVariant dst)
{
    try
    {
        return std::visit([&dst](auto&& arg) { return arg->connect(dst); }, src);
    }
    catch (const std::bad_variant_access&)
    {
        return false;
    }
}

} // namespace lm