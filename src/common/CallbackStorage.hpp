#pragma once

template <typename T, typename Return, typename... Args>
concept ConvertibleToFunctionPtr = std::convertible_to<T, Return (*)(Args...)>;

template <typename ReturnType, typename... Args> class CallbackStorage
{
    void* object = nullptr;
    ReturnType (*callback)(void*, CallbackStorage*, Args...) = nullptr;

public:
    CallbackStorage() = default;

    template <typename F>
        requires ConvertibleToFunctionPtr<F, ReturnType, void*, CallbackStorage*, Args...>
    void setCallback(void* object, F callback) noexcept
    {
        this->object = object;
        this->callback = callback;
    }

    ReturnType triggerCallback(Args... args)
    {
        if (object && object)
        {
            return callback(object, this, args...);
        }

        return ReturnType{};
    }
};
