#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <vector>

#include <orderbook/common/ObjectPool.hpp>

using namespace lm;

TEST(TestObjectPool, InsertDelete)
{
    ObjectPool<uint64_t, 1000> pool;

    std::random_device rd;
    std::mt19937 generator(rd());

    for (size_t k = 0; k < 10; k++)
    {
        std::vector<uint64_t*> allocated;

        for (size_t i = 0; i < 1000; i++)
        {
            auto ptr = pool.allocate();
            *ptr = i;

            ASSERT_NE(nullptr, ptr);
            allocated.push_back(ptr);
        }

        auto ptr = pool.allocate();
        ASSERT_EQ(nullptr, ptr);

        std::shuffle(allocated.begin(), allocated.end(), generator);

        for (auto& ptr : allocated)
        {
            pool.free(ptr);
        }
    }
}
