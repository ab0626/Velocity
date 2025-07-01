#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace velocity;

// ============================================================================
// INVALID/MALFORMED ORDER TESTS
// ============================================================================
void test_invalid_orders() {
    std::cout << "Testing Invalid/Malformed Orders...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    // Zero quantity
    Order zero_qty;
    zero_qty.symbol = "AAPL";
    zero_qty.side = OrderSide::BUY;
    zero_qty.type = OrderType::LIMIT;
    zero_qty.price = 150.0;
    zero_qty.quantity = 0;
    zero_qty.trader_id = "INVALID";
    assert(order_manager->place_order(zero_qty) == 0);
    // Negative price
    Order neg_price;
    neg_price.symbol = "AAPL";
    neg_price.side = OrderSide::SELL;
    neg_price.type = OrderType::LIMIT;
    neg_price.price = -10.0;
    neg_price.quantity = 100;
    neg_price.trader_id = "INVALID";
    assert(order_manager->place_order(neg_price) == 0);
    // Missing symbol
    Order no_symbol;
    no_symbol.symbol = "";
    no_symbol.side = OrderSide::BUY;
    no_symbol.type = OrderType::LIMIT;
    no_symbol.price = 150.0;
    no_symbol.quantity = 100;
    no_symbol.trader_id = "INVALID";
    assert(order_manager->place_order(no_symbol) == 0);
    // Invalid side/type (simulate by casting)
    Order bad_side;
    bad_side.symbol = "AAPL";
    bad_side.side = static_cast<OrderSide>(-1);
    bad_side.type = static_cast<OrderType>(-1);
    bad_side.price = 150.0;
    bad_side.quantity = 100;
    bad_side.trader_id = "INVALID";
    assert(order_manager->place_order(bad_side) == 0);
    std::cout << "Invalid/Malformed Order tests passed!\n";
}

// ============================================================================
// RISK LIMIT VIOLATION TESTS
// ============================================================================
void test_risk_limit_violations() {
    std::cout << "Testing Risk Limit Violations...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    RiskLimits limits;
    limits.max_order_size = 1000;
    limits.max_position_value = 10000.0;
    limits.max_daily_loss = 1000.0;
    order_manager->set_risk_limits(limits);
    // Exceed max order size
    Order big_order;
    big_order.symbol = "AAPL";
    big_order.side = OrderSide::BUY;
    big_order.type = OrderType::LIMIT;
    big_order.price = 150.0;
    big_order.quantity = 5000;
    big_order.trader_id = "RISK";
    assert(order_manager->place_order(big_order) == 0);
    // Exceed max position value
    Order big_value;
    big_value.symbol = "AAPL";
    big_value.side = OrderSide::BUY;
    big_value.type = OrderType::LIMIT;
    big_value.price = 1000.0;
    big_value.quantity = 20;
    big_value.trader_id = "RISK";
    assert(order_manager->place_order(big_value) == 0);
    std::cout << "Risk Limit Violation tests passed!\n";
}

// ============================================================================
// ORDER BOOK EDGE CASES
// ============================================================================
void test_order_book_edge_cases() {
    std::cout << "Testing Order Book Edge Cases...\n";
    OrderBook book("AAPL");
    // Empty book
    assert(book.get_best_bid() == 0.0);
    assert(book.get_best_ask() == 0.0);
    assert(book.get_spread() == 0.0);
    // Extreme spread
    Order buy;
    buy.symbol = "AAPL";
    buy.side = OrderSide::BUY;
    buy.type = OrderType::LIMIT;
    buy.price = 100.0;
    buy.quantity = 100;
    book.add_order(buy);
    Order sell;
    sell.symbol = "AAPL";
    sell.side = OrderSide::SELL;
    sell.type = OrderType::LIMIT;
    sell.price = 1000.0;
    sell.quantity = 100;
    book.add_order(sell);
    assert(book.get_spread() == 900.0);
    // Duplicate/cancelled orders
    book.cancel_order(999999); // Non-existent order
    // Add and cancel
    Order cancel_me;
    cancel_me.symbol = "AAPL";
    cancel_me.side = OrderSide::BUY;
    cancel_me.type = OrderType::LIMIT;
    cancel_me.price = 120.0;
    cancel_me.quantity = 10;
    cancel_me.id = 12345;
    book.add_order(cancel_me);
    book.cancel_order(12345);
    std::cout << "Order Book Edge Case tests passed!\n";
}

// ============================================================================
// SIMULATED FAILURES & INVALID SYMBOLS
// ============================================================================
void test_simulated_failures() {
    std::cout << "Testing Simulated Failures & Invalid Symbols...\n";
    auto order_manager = std::make_shared<OrderManager>();
    // Invalid symbol
    try {
        order_manager->get_order_book("INVALID");
        assert(false && "Should throw for invalid symbol");
    } catch (const std::exception&) {
        // Expected
    }
    // MarketDataFeed invalid symbol
    MarketDataFeed feed;
    feed.add_symbol("AAPL", 150.0);
    try {
        feed.get_order_book("INVALID");
        assert(false && "Should throw for invalid symbol");
    } catch (const std::exception&) {
        // Expected
    }
    std::cout << "Simulated Failure & Invalid Symbol tests passed!\n";
}

// ============================================================================
// MAIN EDGE CASE TEST RUNNER
// ============================================================================
int main() {
    std::cout << "Running Edge Case & Error Handling Tests for Velocity HFT Simulator...\n\n";
    try {
        test_invalid_orders();
        test_risk_limit_violations();
        test_order_book_edge_cases();
        test_simulated_failures();
        std::cout << "\n⚠️  All Edge Case & Error Handling tests passed! ⚠️\n";
        std::cout << "Velocity HFT Simulator is robust against edge cases.\n";
    } catch (const std::exception& e) {
        std::cerr << "Edge case test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 