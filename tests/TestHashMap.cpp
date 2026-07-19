#include <gtest/gtest.h>

#include "common/HashMap.hpp"

using namespace lm;

TEST(TestHashMap, Basic)
{
    HashMap<uint64_t, uint64_t, 3> hash_map;

    hash_map.insert(4, 1);
    hash_map.insert(4, 10);
    hash_map.insert(5, 2);
    hash_map.insert(6, 3);
    hash_map.insert(7, 4);
    hash_map.insert(8, 5);

    ASSERT_EQ(10, hash_map.get(4));
    ASSERT_EQ(2, hash_map.get(5));
    ASSERT_EQ(3, hash_map.get(6));
    ASSERT_EQ(4, hash_map.get(7));
    ASSERT_EQ(5, hash_map.get(8));

    ASSERT_TRUE(hash_map.contains(8));
    ASSERT_FALSE(hash_map.get(9));

    ASSERT_EQ(5, hash_map.size());

    hash_map.erase(4);
    ASSERT_EQ(4, hash_map.size());

    hash_map.erase(4);
    ASSERT_EQ(4, hash_map.size());
}