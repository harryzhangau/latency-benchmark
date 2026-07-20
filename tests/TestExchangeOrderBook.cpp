
#include <gtest/gtest.h>
#include <memory>

#include <common/SpscMessageBus.hpp>
#include <common/Utils.hpp>
#include <devices/OrderGeneratorDevice.hpp>
#include <exchange/ExchangeOrderBook.hpp>
#include <exchange/ExchangeOrders.hpp>
#include <exchange/ItchMessage.hpp>
#include <exchange/OrderMessage.hpp>

using namespace lm;

class TestExchangeOrderBook : public testing::Test
{
public:
    size_t getOrderBookOrderCount(auto& orderBook) const
    {
        return orderBook.orderMap.size();
    }
};

TEST_F(TestExchangeOrderBook, TestHigherBitsMask)
{
    ASSERT_EQ(0xffff'ffff'ffff'fffe, higherBitsMask(0));
    ASSERT_EQ(0xffff'ffff'ffff'fffc, higherBitsMask(1));
    ASSERT_EQ(0x0000'0000'0000'0000, higherBitsMask(63));
}

TEST_F(TestExchangeOrderBook, TestLowerBitsMask)
{
    ASSERT_EQ(0x0000'0000'0000'0000, lowerBitsMask(0));
    ASSERT_EQ(0x0000'0000'0000'0001, lowerBitsMask(1));
    ASSERT_EQ(0x7fff'ffff'ffff'ffff, lowerBitsMask(63));
}

TEST_F(TestExchangeOrderBook, TestBitmap)
{
    static constexpr size_t N = 4;
    std::array<uint64_t, N> bitmap;
    memset(&bitmap[0], 0, sizeof(bitmap));
    static_assert(sizeof(bitmap) == N * 8);

    setBitmap(bitmap, 64);
    ASSERT_EQ(0x0, bitmap[0]);
    ASSERT_EQ(0x1, bitmap[1]);

    clearBitmap(bitmap, 64);
    ASSERT_EQ(0x0, bitmap[0]);
    ASSERT_EQ(0x0, bitmap[1]);

    setBitmap(bitmap, 65);
    setBitmap(bitmap, 66);
    ASSERT_EQ(0x0, bitmap[0]);
    ASSERT_EQ(0x6, bitmap[1]);

    clearBitmap(bitmap, 65);
    ASSERT_EQ(0x0, bitmap[0]);
    ASSERT_EQ(0x4, bitmap[1]);
}

TEST_F(TestExchangeOrderBook, TestBuySide)
{
    ASSERT_TRUE(BuySide::betterOrSame(1000, 900));
    ASSERT_TRUE(BuySide::betterOrSame(900, 900));

    ASSERT_EQ(1, BuySide::closestBetterPricePos(0x2, 0));
    ASSERT_EQ(2, BuySide::closestBetterPricePos(0x4, 0));
    ASSERT_EQ(63, BuySide::closestBetterPricePos(0x8000'0000'0000'ffff, 20));
    ASSERT_EQ(64, BuySide::closestBetterPricePos(0x8000'0000'0000'ffff, 63));

    ASSERT_EQ(0, BuySide::closestBetterPricePos(0x8000'0000'0000'ffff));
    ASSERT_EQ(63, BuySide::closestBetterPricePos(0x8000'0000'0000'0000));

    int index = 10;
    ASSERT_TRUE(BuySide::nextBetter(index, 32));
    ASSERT_EQ(11, index);
    index = 32;
    ASSERT_FALSE(BuySide::nextBetter(index, 32));
}

TEST_F(TestExchangeOrderBook, TestSellSide)
{
    ASSERT_TRUE(SellSide::betterOrSame(900, 1000));
    ASSERT_TRUE(SellSide::betterOrSame(900, 900));

    ASSERT_EQ(1, SellSide::closestBetterPricePos(0x2, 10));
    ASSERT_EQ(2, SellSide::closestBetterPricePos(0x4, 10));
    ASSERT_EQ(15, SellSide::closestBetterPricePos(0x8000'0000'0000'ffff, 20));
    ASSERT_EQ(15, SellSide::closestBetterPricePos(0x8000'0000'0000'ffff, 63));
    ASSERT_EQ(-1, SellSide::closestBetterPricePos(0x8000'0000'0000'ffff, 0));

    ASSERT_EQ(63, SellSide::closestBetterPricePos(0x8000'0000'0000'ffff));
    ASSERT_EQ(63, SellSide::closestBetterPricePos(0x8000'0000'0000'0000));

    int index = 10;
    ASSERT_TRUE(SellSide::nextBetter(index, 32));
    ASSERT_EQ(9, index);
    index = 0;
    ASSERT_FALSE(SellSide::nextBetter(index, 32));
}

TEST_F(TestExchangeOrderBook, TestOrders)
{
    using Asks = ExchangeOrders<SellSide, ExchangeOrderBook::LinkedExchangeOrderAllocator,
                                ExchangeOrderBook::LinkedExchangeOrderDeleter>;

    using Bids = ExchangeOrders<BuySide, ExchangeOrderBook::LinkedExchangeOrderAllocator,
                                ExchangeOrderBook::LinkedExchangeOrderDeleter>;

    auto asks = std::make_unique<Asks>();
    asks->insert({0, AddOrder::Side::SELL, 355, 149});
    auto remaining = asks->match({1, AddOrder::Side::BUY, 375, 113}, [](auto) {});
    ASSERT_EQ(0, remaining);
    remaining = asks->match({2, AddOrder::Side::BUY, 386, 123}, [](auto) {});
    ASSERT_EQ(87, remaining);

    auto bids = std::make_unique<Bids>();
    bids->insert({3, AddOrder::Side::BUY, 355, 149});
    remaining = bids->match({4, AddOrder::Side::SELL, 310, 113}, [](auto) {});
    ASSERT_EQ(0, remaining);
    remaining = bids->match({5, AddOrder::Side::BUY, 320, 123}, [](auto) {});
    ASSERT_EQ(87, remaining);
}

TEST_F(TestExchangeOrderBook, TestOrderBook)
{
    auto message_bus = std::make_unique<SpscMessageBus<itch::ItchMessage>>();
    auto exchange_order_book = std::make_unique<ExchangeOrderBook>();

    auto order1 = AddOrder{100, AddOrder::Side::BUY, 355, 149};
    auto order2 = AddOrder{101, AddOrder::Side::BUY, 320, 190};

    exchange_order_book->processOrderMessage(order1);
    exchange_order_book->processOrderMessage(order2);

    ASSERT_EQ(2, getOrderBookOrderCount(*exchange_order_book));

    auto cancelOrderMsg = DeleteOrder{100};
    exchange_order_book->processOrderMessage(cancelOrderMsg);
    ASSERT_EQ(1, getOrderBookOrderCount(*exchange_order_book));
}