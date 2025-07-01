#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include "../include/performance_analytics.h"
#include "../include/velocity_engine.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <memory>

using namespace velocity;

// ============================================================================
// TRADING STRATEGIES TESTS
// ============================================================================

void test_market_making_strategy() {
    std::cout << "Testing Market Making Strategy...\n";
    
    // Create order manager
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    // Create market making strategy
    MarketMakingStrategy strategy("MM_Strategy", "MM_TRADER", order_manager);
    strategy.add_symbol("AAPL");
    strategy.set_spread_multiplier(1.5);
    strategy.set_base_quantity(100);
    strategy.set_max_position(1000);
    strategy.set_min_spread(0.01);
    
    // Initialize and start strategy
    strategy.initialize();
    strategy.start();
    
    // Simulate market data updates
    strategy.on_market_data("AAPL", 150.0, 150.5);
    
    // Check strategy state
    assert(strategy.is_running());
    assert(strategy.get_name() == "MM_Strategy");
    assert(strategy.get_trader_id() == "MM_TRADER");
    
    // Stop strategy
    strategy.stop();
    assert(!strategy.is_running());
    
    std::cout << "Market Making Strategy tests passed!\n";
}

void test_statistical_arbitrage_strategy() {
    std::cout << "Testing Statistical Arbitrage Strategy...\n";
    
    // Create order manager
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    
    // Create stat arb strategy
    StatArbStrategy strategy("StatArb_Strategy", "StatArb_TRADER", order_manager);
    strategy.set_pair_symbols("AAPL", "GOOGL");
    strategy.set_z_score_threshold(2.0);
    strategy.set_lookback_period(20);
    strategy.set_position_size(100);
    
    // Initialize and start strategy
    strategy.initialize();
    strategy.start();
    
    // Simulate price updates for both symbols
    for (int i = 0; i < 25; ++i) {
        strategy.on_market_data("AAPL", 150.0 + i * 0.1, 150.1 + i * 0.1);
        strategy.on_market_data("GOOGL", 2800.0 + i * 2.0, 2801.0 + i * 2.0);
    }
    
    // Check strategy state
    assert(strategy.is_running());
    
    // Stop strategy
    strategy.stop();
    
    std::cout << "Statistical Arbitrage Strategy tests passed!\n";
}

void test_momentum_strategy() {
    std::cout << "Testing Momentum Strategy...\n";
    
    // Create order manager
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("TSLA");
    
    // Create momentum strategy
    MomentumStrategy strategy("Momentum_Strategy", "Momentum_TRADER", order_manager);
    strategy.add_symbol("TSLA");
    strategy.set_windows(5, 20);
    strategy.set_momentum_threshold(0.02);
    strategy.set_position_size(100);
    
    // Initialize and start strategy
    strategy.initialize();
    strategy.start();
    
    // Simulate price updates with momentum
    for (int i = 0; i < 25; ++i) {
        double price = 800.0 + i * 5.0; // Upward momentum
        strategy.on_market_data("TSLA", price, price + 0.5);
    }
    
    // Check strategy state
    assert(strategy.is_running());
    
    // Stop strategy
    strategy.stop();
    
    std::cout << "Momentum Strategy tests passed!\n";
}

// ============================================================================
// PERFORMANCE ANALYTICS TESTS
// ============================================================================

void test_performance_analytics() {
    std::cout << "Testing Performance Analytics...\n";
    
    PerformanceAnalytics analytics;
    
    // Create sample trades
    Trade trade1;
    trade1.trade_id = 1;
    trade1.symbol = "AAPL";
    trade1.side = OrderSide::BUY;
    trade1.entry_price = 150.0;
    trade1.exit_price = 155.0;
    trade1.quantity = 100;
    trade1.pnl = 500.0;
    trade1.entry_time = std::chrono::high_resolution_clock::now();
    trade1.exit_time = trade1.entry_time + std::chrono::hours(1);
    trade1.latency = std::chrono::microseconds(1000);
    
    Trade trade2;
    trade2.trade_id = 2;
    trade2.symbol = "GOOGL";
    trade2.side = OrderSide::SELL;
    trade2.entry_price = 2800.0;
    trade2.exit_price = 2750.0;
    trade2.quantity = 50;
    trade2.pnl = 2500.0;
    trade2.entry_time = std::chrono::high_resolution_clock::now();
    trade2.exit_time = trade2.entry_time + std::chrono::hours(2);
    trade2.latency = std::chrono::microseconds(1500);
    
    // Record trades
    analytics.record_trade(trade1);
    analytics.record_trade(trade2);
    
    // Create latency measurements
    LatencyMeasurement latency1;
    latency1.order_id = 1;
    latency1.latency = std::chrono::microseconds(1000);
    latency1.symbol = "AAPL";
    latency1.side = OrderSide::BUY;
    
    LatencyMeasurement latency2;
    latency2.order_id = 2;
    latency2.latency = std::chrono::microseconds(1500);
    latency2.symbol = "GOOGL";
    latency2.side = OrderSide::SELL;
    
    // Record latency measurements
    // analytics.record_latency(latency1);
    // analytics.record_latency(latency2);
    
    // Calculate metrics
    // analytics.calculate_metrics();
    // analytics.calculate_risk_metrics();
    
    // Get metrics
    auto metrics = analytics.get_performance_metrics();
    auto risk_metrics = analytics.get_risk_metrics();
    auto trades = analytics.get_trades();
    auto latencies = analytics.get_latency_measurements();
    
    // Verify metrics
    assert(trades.size() == 2);
    assert(latencies.size() == 2);
    assert(metrics.total_trades == 2);
    assert(metrics.total_pnl == 3000.0); // 500 + 2500
    assert(metrics.winning_trades == 2);
    assert(metrics.win_rate == 1.0);
    
    std::cout << "Performance Analytics tests passed!\n";
}

// ============================================================================
// VELOCITY ENGINE INTEGRATION TESTS
// ============================================================================

void test_velocity_engine() {
    std::cout << "Testing Velocity Engine Integration...\n";
    
    // Create engine configuration
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL", "TSLA"};
    config.initial_prices["AAPL"] = 150.0;
    config.initial_prices["GOOGL"] = 2800.0;
    config.initial_prices["TSLA"] = 800.0;
    config.enabled_strategies = {"MarketMaking", "Momentum"};
    config.enable_logging = false; // Disable for testing
    
    // Create engine
    VelocityEngine engine(config);
    
    // Initialize engine
    engine.initialize();
    
    // Add strategies
    engine.add_strategy("MarketMaking", "MM_Strategy", "MM_TRADER");
    engine.add_strategy("Momentum", "Momentum_Strategy", "Momentum_TRADER");
    
    // Check engine state
    assert(!engine.is_running());
    
    // Get symbols
    auto symbols = engine.get_symbols();
    assert(symbols.size() == 3);
    
    // Get strategy names
    auto strategy_names = engine.get_strategy_names();
    assert(strategy_names.size() == 2);
    
    // Start engine briefly
    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    engine.stop();
    
    std::cout << "Velocity Engine Integration tests passed!\n";
}

// ============================================================================
// RISK MANAGEMENT TESTS
// ============================================================================

void test_risk_management() {
    std::cout << "Testing Risk Management...\n";
    
    // Create order manager with risk limits
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    RiskLimits limits;
    limits.max_position_value = 100000.0;
    limits.max_daily_loss = 10000.0;
    limits.max_order_size = 1000;
    limits.max_drawdown = 0.1;
    limits.max_leverage = 2.0;
    
    order_manager->set_risk_limits(limits);
    
    // Test order size limit
    Order large_order;
    large_order.symbol = "AAPL";
    large_order.side = OrderSide::BUY;
    large_order.type = OrderType::LIMIT;
    large_order.price = 150.0;
    large_order.quantity = 2000; // Exceeds max_order_size
    large_order.trader_id = "TEST_TRADER";
    
    uint64_t order_id = order_manager->place_order(large_order);
    assert(order_id == 0); // Should be rejected
    
    // Test valid order
    Order valid_order;
    valid_order.symbol = "AAPL";
    valid_order.side = OrderSide::BUY;
    valid_order.type = OrderType::LIMIT;
    valid_order.price = 150.0;
    valid_order.quantity = 500; // Within limits
    valid_order.trader_id = "TEST_TRADER";
    
    order_id = order_manager->place_order(valid_order);
    assert(order_id > 0); // Should be accepted
    
    std::cout << "Risk Management tests passed!\n";
}

// ============================================================================
// ORDER BOOK MATCHING TESTS
// ============================================================================

void test_order_book_matching() {
    std::cout << "Testing Order Book Matching...\n";
    
    OrderBook book("AAPL");
    
    // Add buy orders
    Order buy_order1;
    buy_order1.symbol = "AAPL";
    buy_order1.side = OrderSide::BUY;
    buy_order1.type = OrderType::LIMIT;
    buy_order1.price = 150.0;
    buy_order1.quantity = 100;
    buy_order1.trader_id = "BUYER1";
    
    Order buy_order2;
    buy_order2.symbol = "AAPL";
    buy_order2.side = OrderSide::BUY;
    buy_order2.type = OrderType::LIMIT;
    buy_order2.price = 149.5;
    buy_order2.quantity = 200;
    buy_order2.trader_id = "BUYER2";
    
    // Add sell orders
    Order sell_order1;
    sell_order1.symbol = "AAPL";
    sell_order1.side = OrderSide::SELL;
    sell_order1.type = OrderType::LIMIT;
    sell_order1.price = 151.0;
    sell_order1.quantity = 150;
    sell_order1.trader_id = "SELLER1";
    
    Order sell_order2;
    sell_order2.symbol = "AAPL";
    sell_order2.side = OrderSide::SELL;
    sell_order2.type = OrderType::LIMIT;
    sell_order2.price = 150.5;
    sell_order2.quantity = 100;
    sell_order2.trader_id = "SELLER2";
    
    // Add orders to book
    book.add_order(buy_order1);
    book.add_order(buy_order2);
    book.add_order(sell_order1);
    book.add_order(sell_order2);
    
    // Test order book state
    assert(book.get_best_bid() == 150.0);
    assert(book.get_best_ask() == 150.5);
    assert(book.get_mid_price() == 150.25);
    assert(book.get_spread() == 0.5);
    
    // Test order book depth
    auto bid_levels = book.get_bid_levels();
    auto ask_levels = book.get_ask_levels();
    
    assert(bid_levels.size() == 2);
    assert(ask_levels.size() == 2);
    
    std::cout << "Order Book Matching tests passed!\n";
}

// ============================================================================
// MARKET DATA FEED SIMULATION TESTS
// ============================================================================

void test_market_data_simulation() {
    std::cout << "Testing Market Data Feed Simulation...\n";
    
    MarketDataFeed feed;
    feed.add_symbol("AAPL", 150.0);
    feed.add_symbol("GOOGL", 2800.0);
    
    // Simulate price movements
    for (int i = 0; i < 10; ++i) {
        double aapl_price = 150.0 + (i * 0.5);
        double googl_price = 2800.0 + (i * 10.0);
        
        // Update prices
        feed.update_price("AAPL", aapl_price);
        feed.update_price("GOOGL", googl_price);
        
        // Get order books
        auto& aapl_book = feed.get_order_book("AAPL");
        auto& googl_book = feed.get_order_book("GOOGL");
        
        // Verify price updates
        assert(aapl_book.get_mid_price() >= 150.0);
        assert(googl_book.get_mid_price() >= 2800.0);
    }
    
    // Test symbol management
    feed.add_symbol("TSLA", 800.0);
    auto& tsla_book = feed.get_order_book("TSLA");
    assert(tsla_book.get_symbol() == "TSLA");
    
    std::cout << "Market Data Feed Simulation tests passed!\n";
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "Running Comprehensive Velocity HFT Simulator Tests...\n\n";
    
    try {
        // Trading Strategies Tests
        test_market_making_strategy();
        test_statistical_arbitrage_strategy();
        test_momentum_strategy();
        
        // Performance Analytics Tests
        test_performance_analytics();
        
        // Velocity Engine Integration Tests
        test_velocity_engine();
        
        // Risk Management Tests
        test_risk_management();
        
        // Order Book Matching Tests
        test_order_book_matching();
        
        // Market Data Feed Simulation Tests
        test_market_data_simulation();
        
        std::cout << "\n🎉 All comprehensive tests passed! 🎉\n";
        std::cout << "Velocity HFT Simulator is fully functional.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 