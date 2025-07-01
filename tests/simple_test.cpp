#include "../include/market_data.h"
#include <iostream>
#include <cassert>

using namespace velocity;

void test_order_book() {
    std::cout << "Testing OrderBook...\n";
    
    OrderBook book("AAPL");
    
    // Test adding orders
    Order buy_order;
    buy_order.symbol = "AAPL";
    buy_order.side = OrderSide::BUY;
    buy_order.type = OrderType::LIMIT;
    buy_order.price = 150.0;
    buy_order.quantity = 1000;
    buy_order.trader_id = "TEST_TRADER";
    
    Order sell_order;
    sell_order.symbol = "AAPL";
    sell_order.side = OrderSide::SELL;
    sell_order.type = OrderType::LIMIT;
    sell_order.price = 151.0;
    sell_order.quantity = 1000;
    sell_order.trader_id = "TEST_TRADER";
    
    book.add_order(buy_order);
    book.add_order(sell_order);
    
    // Test order book queries
    assert(book.get_best_bid() == 150.0);
    assert(book.get_best_ask() == 151.0);
    assert(book.get_mid_price() == 150.5);
    assert(book.get_spread() == 1.0);
    
    std::cout << "OrderBook tests passed!\n";
}

void test_market_data_feed() {
    std::cout << "Testing MarketDataFeed...\n";
    
    MarketDataFeed feed;
    feed.add_symbol("AAPL", 150.0);
    feed.add_symbol("GOOGL", 2800.0);
    
    // Test order book access
    auto& aapl_book = feed.get_order_book("AAPL");
    auto& googl_book = feed.get_order_book("GOOGL");
    
    assert(aapl_book.get_symbol() == "AAPL");
    assert(googl_book.get_symbol() == "GOOGL");
    
    std::cout << "MarketDataFeed tests passed!\n";
}

void test_order_creation() {
    std::cout << "Testing Order creation...\n";
    
    Order order;
    order.symbol = "TSLA";
    order.side = OrderSide::BUY;
    order.type = OrderType::MARKET;
    order.price = 800.0;
    order.quantity = 500;
    order.trader_id = "TEST_TRADER";
    
    assert(order.symbol == "TSLA");
    assert(order.side == OrderSide::BUY);
    assert(order.type == OrderType::MARKET);
    assert(order.price == 800.0);
    assert(order.quantity == 500);
    assert(order.trader_id == "TEST_TRADER");
    
    std::cout << "Order creation tests passed!\n";
}

void test_price_level() {
    std::cout << "Testing PriceLevel...\n";
    
    PriceLevel level(150.0);
    assert(level.price == 150.0);
    assert(level.total_quantity == 0);
    assert(level.orders.empty());
    
    Order order;
    order.price = 150.0;
    order.quantity = 1000;
    
    level.orders.push_back(order);
    level.total_quantity += order.quantity;
    
    assert(level.total_quantity == 1000);
    assert(level.orders.size() == 1);
    
    std::cout << "PriceLevel tests passed!\n";
}

int main() {
    std::cout << "Running Velocity HFT Simulator tests...\n\n";
    
    try {
        test_order_creation();
        test_price_level();
        test_order_book();
        test_market_data_feed();
        
        std::cout << "\nAll tests passed! ✅\n";
        std::cout << "Velocity HFT Simulator is ready to use.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 