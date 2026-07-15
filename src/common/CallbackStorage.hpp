#pragma once

template <typename T, typename Return, typename... Args>
concept ConvertibleToFunctionPtr = std::convertible_to<T, Return (*)(Args...)>;

class CallbackStorage
{
    void* object = nullptr;
    void (*callback)(void*, CallbackStorage*) = nullptr;

public:
    CallbackStorage() = default;

    template <typename F>
        requires ConvertibleToFunctionPtr<F, void, void*, CallbackStorage*>
    void setCallback(void* object, F callback) noexcept
    {
        this->object = object;
        this->callback = callback;
    }

    void triggerCallback()
    {
        if (object && object)
        {
            callback(object, this);
        }
    }
};
