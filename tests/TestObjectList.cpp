#include <gtest/gtest.h>

#include <orderbook/common/ObjectList.hpp>

using namespace lm;

struct TestElement
{
    TestElement* next = nullptr;
    TestElement* prev = nullptr;
};

TEST(TestObjectList, Basic)
{
    ObjectList<TestElement> list;

    ASSERT_TRUE(list.empty());
    list.push(new TestElement());
    ASSERT_FALSE(list.empty());
    list.erase(list.front());
    ASSERT_TRUE(list.empty());

    for (size_t i = 0; i < 100; i++)
    {
        list.push(new TestElement());
    }

    for (size_t i = 0; i < 50; i++)
    {
        list.erase(list.front());
    }

    int count = 0;
    auto element = list.front();
    while (element != nullptr)
    {
        element = element->next;
        count++;
    }
    ASSERT_EQ(50, count);
    ASSERT_FALSE(list.empty());

    element = list.front();
    while (element != nullptr)
    {
        element = list.erase(element);
        count++;
    }

    ASSERT_EQ(100, count);
    ASSERT_TRUE(list.empty());
}
