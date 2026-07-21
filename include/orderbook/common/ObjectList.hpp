#pragma once

#include <concepts>
#include <memory>

namespace lm
{

template <typename T>
concept HasLinkMembers = requires(T v) {
    { v.prev } -> std::same_as<T*&>;
    { v.next } -> std::same_as<T*&>;
};

template <HasLinkMembers T, typename Deleter = std::default_delete<T>> class ObjectList
{
public:
    ObjectList(Deleter deleter = Deleter())
        : deleter(deleter)
    {
    }

    ~ObjectList()
    {
        while (head != nullptr)
        {
            auto next = head->next;
            deleter(head);
            head = next;
        }
    }

    void push(T* element) noexcept
    {
        if (head == nullptr)
        {
            head = element;
            tail = element;
        }
        else
        {
            tail->next = element;
            element->prev = tail;
            element->next = nullptr;

            tail = element;
        }
    }

    T* front() noexcept
    {
        return head;
    }

    T* erase(T* element) noexcept
    {
        if (element->next)
        {
            element->next->prev = element->prev;
        }

        if (element->prev)
        {
            element->prev->next = element->next;
        }

        auto next = element->next;
        if (head == element)
        {
            head = next;
        }

        if (tail == element)
        {
            tail = element->prev;
        }

        deleter(element);
        return next;
    }

    bool empty() const noexcept
    {
        return head == nullptr;
    }

private:
    T* head = nullptr;
    T* tail = nullptr;

    Deleter deleter;
};

} // namespace lm
