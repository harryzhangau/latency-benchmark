#pragma once

#include <array>
#include <stdint.h>
#include <unordered_set>

namespace lm
{

template <typename T>
concept LargeType = sizeof(T) >= sizeof(void*);

template <LargeType T, size_t N> class ObjectPool
{
public:
    class Deleter
    {
    public:
        Deleter(ObjectPool& pool)
            : pool(pool)
        {
        }

        void operator()(T* order)
        {
            if (order == nullptr)
                return;

            pool.free(order);
        }

    private:
        ObjectPool& pool;
    };

    ObjectPool()
    {
        freeList = reinterpret_cast<FreeNode*>(&pool[0]);
        freeList->next = nullptr;

        for (size_t i = 1; i < N; i++)
        {
            auto current = reinterpret_cast<FreeNode*>(&pool[i]);
            current->next = freeList;
            freeList = current;
        }
    }

    T* allocate()
    {
        if (freeList == nullptr)
        {
            return nullptr;
        }

        T* object = reinterpret_cast<T*>(freeList);
        freeList = freeList->next;

        return object;
    }

    void free(T* object)
    {
        if (object == nullptr)
            return;

        auto new_free_node = reinterpret_cast<FreeNode*>(object);
        new_free_node->next = freeList;
        freeList = new_free_node;
    }

    T* memory()
    {
        return &pool[0];
    }

private:
    struct FreeNode
    {
        FreeNode* next = nullptr;
    };

    std::array<T, N> pool;
    FreeNode* freeList = nullptr;
};

} // namespace lm
