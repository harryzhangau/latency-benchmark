#pragma once

template <typename T, typename Return, typename... Args>
concept ConvertibleToFunctionPtr = std::convertible_to<T, Return (*)(Args...)>;

template <typename ReturnType, typename... Args> class CallbackStorage
{
    void* delegate = nullptr;
    ReturnType (*callback)(void*, CallbackStorage*, Args...) = nullptr;

public:
    CallbackStorage() = default;

    template <typename F>
        requires ConvertibleToFunctionPtr<F, ReturnType, void*, CallbackStorage*, Args...>
    void setCallback(void* delegate, F callback) noexcept
    {
        this->delegate = delegate;
        this->callback = callback;
    }

    ReturnType triggerCallback(Args... args)
    {
        return callback(delegate, this, args...);
    }
};
