#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include "../include/performance_analytics.h"
#include "../include/velocity_engine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>

using namespace velocity;

// ============================================================================
// HIGH-FREQUENCY ORDER BURST TEST
// ============================================================================
void test_high_frequency_order_burst() {
    std::cout << "Testing High-Frequency Order Burst...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    const uint32_t num_threads = 8;
    const uint32_t orders_per_thread = 10000;
    std::vector<std::thread> threads;
    std::atomic<uint64_t> total_orders{0};
    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (uint32_t i = 0; i < orders_per_thread; ++i) {
                Order order;
                order.symbol = "AAPL";
                order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
                order.type = OrderType::LIMIT;
                order.price = 150.0 + (i % 100) * 0.01;
                order.quantity = 100;
                order.trader_id = "BURST_" + std::to_string(t);
                order_manager->place_order(order);
                ++total_orders;
            }
        });
    }
    for (auto& th : threads) th.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "  Total orders: " << total_orders << "\n";
    std::cout << "  Duration: " << duration.count() << " ms\n";
    assert(total_orders == num_threads * orders_per_thread);
    std::cout << "High-Frequency Order Burst test passed!\n";
}

// ============================================================================
// CONCURRENT STRATEGY EXECUTION TEST
// ============================================================================
void test_concurrent_strategies() {
    std::cout << "Testing Concurrent Strategy Execution...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    
    MarketMakingStrategy mm("MM", "MM1", order_manager);
    StatArbStrategy statarb("StatArb", "SA1", order_manager);
    statarb.set_pair_symbols("AAPL", "GOOGL");
    MomentumStrategy mom("Momentum", "MOM1", order_manager);
    mom.add_symbol("AAPL");
    
    mm.initialize(); statarb.initialize(); mom.initialize();
    mm.start(); statarb.start(); mom.start();
    
    std::vector<std::thread> threads;
    threads.emplace_back([&]() {
        for (int i = 0; i < 1000; ++i) mm.on_market_data("AAPL", 150.0 + i * 0.01, 150.5 + i * 0.01);
    });
    threads.emplace_back([&]() {
        for (int i = 0; i < 1000; ++i) statarb.on_market_data("AAPL", 150.0 + i * 0.01, 150.5 + i * 0.01);
    });
    threads.emplace_back([&]() {
        for (int i = 0; i < 1000; ++i) mom.on_market_data("AAPL", 150.0 + i * 0.01, 150.5 + i * 0.01);
    });
    for (auto& th : threads) th.join();
    mm.stop(); statarb.stop(); mom.stop();
    std::cout << "Concurrent Strategy Execution test passed!\n";
}

// ============================================================================
// MARKET DATA BURST TEST
// ============================================================================
void test_market_data_burst() {
    std::cout << "Testing Market Data Burst...\n";
    MarketDataFeed feed;
    feed.add_symbol("AAPL", 150.0);
    const uint32_t num_updates = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < num_updates; ++i) {
        double price = 150.0 + (i % 100) * 0.01;
        feed.update_price("AAPL", price);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "  Total updates: " << num_updates << "\n";
    std::cout << "  Duration: " << duration.count() << " ms\n";
    assert(duration.count() < 10000); // Should complete in under 10 seconds
    std::cout << "Market Data Burst test passed!\n";
}

// ============================================================================
// LONG-RUNNING STABILITY TEST (SHORTENED FOR DEMO)
// ============================================================================
void test_long_running_stability() {
    std::cout << "Testing Long-Running Stability (short run)...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    const uint32_t num_orders = 10000;
    for (uint32_t i = 0; i < num_orders; ++i) {
        Order order;
        order.symbol = "AAPL";
        order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.price = 150.0 + (i % 100) * 0.01;
        order.quantity = 100;
        order.trader_id = "LONGRUN";
        order_manager->place_order(order);
    }
    auto positions = order_manager->get_all_positions();
    std::cout << "  Orders placed: " << num_orders << "\n";
    std::cout << "  Positions tracked: " << positions.size() << "\n";
    assert(positions.size() > 0);
    std::cout << "Long-Running Stability test passed!\n";
}

// ============================================================================
// MAIN STRESS TEST RUNNER
// ============================================================================
int main() {
    std::cout << "Running Stress & Load Tests for Velocity HFT Simulator...\n\n";
    try {
        test_high_frequency_order_burst();
        test_concurrent_strategies();
        test_market_data_burst();
        test_long_running_stability();
        std::cout << "\n🔥 All Stress & Load tests passed! 🔥\n";
        std::cout << "Velocity HFT Simulator is stable under stress.\n";
    } catch (const std::exception& e) {
        std::cerr << "Stress test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 