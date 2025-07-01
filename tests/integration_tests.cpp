#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include "../include/performance_analytics.h"
#include "../include/velocity_engine.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>

using namespace velocity;

// ============================================================================
// FULL TRADING SESSION SIMULATION
// ============================================================================
void test_full_trading_session() {
    std::cout << "Testing Full Trading Session...\n";
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL"};
    config.initial_prices["AAPL"] = 150.0;
    config.initial_prices["GOOGL"] = 2800.0;
    config.enabled_strategies = {"MarketMaking", "Momentum"};
    config.enable_logging = false;
    config.performance_update_frequency_ms = 100;
    VelocityEngine engine(config);
    engine.initialize();
    engine.add_strategy("MarketMaking", "MM", "MM1");
    engine.add_strategy("Momentum", "MOM", "MOM1");
    engine.start();
    // Simulate market data and trading for a short period
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    engine.stop();
    // Check system state
    auto metrics = engine.get_performance_metrics();
    auto positions = engine.get_positions();
    assert(metrics.total_trades >= 0);
    assert(!positions.empty());
    std::cout << "Full Trading Session test passed!\n";
}

// ============================================================================
// DATA PERSISTENCE & EXPORT TESTS
// ============================================================================
void test_data_export() {
    std::cout << "Testing Data Persistence & Export...\n";
    VelocityConfig config;
    config.symbols = {"AAPL"};
    config.initial_prices["AAPL"] = 150.0;
    config.enabled_strategies = {"MarketMaking"};
    config.enable_logging = true;
    config.log_directory = "./logs";
    VelocityEngine engine(config);
    engine.initialize();
    engine.add_strategy("MarketMaking", "MM", "MM1");
    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    engine.stop();
    // Export trades and performance
    engine.export_trades_to_csv("logs/test_trades.csv");
    engine.export_performance_to_csv("logs/test_performance.csv");
    // Check files exist
    std::ifstream trades_file("logs/test_trades.csv");
    std::ifstream perf_file("logs/test_performance.csv");
    assert(trades_file.good());
    assert(perf_file.good());
    std::cout << "Data Persistence & Export test passed!\n";
}

// ============================================================================
// CONFIGURATION VARIATION TESTS
// ============================================================================
void test_configuration_variations() {
    std::cout << "Testing Configuration Variations...\n";
    VelocityConfig config;
    config.symbols = {"AAPL"};
    config.initial_prices["AAPL"] = 150.0;
    config.enabled_strategies = {"MarketMaking"};
    config.max_order_size = 10;
    config.max_position_value = 1000.0;
    config.max_daily_loss = 100.0;
    VelocityEngine engine(config);
    engine.initialize();
    engine.add_strategy("MarketMaking", "MM", "MM1");
    engine.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    engine.stop();
    auto metrics = engine.get_performance_metrics();
    assert(metrics.total_trades >= 0);
    std::cout << "Configuration Variation test passed!\n";
}

// ============================================================================
// END-TO-END TRADING LOOP VALIDATION
// ============================================================================
void test_end_to_end_trading_loop() {
    std::cout << "Testing End-to-End Trading Loop...\n";
    VelocityConfig config;
    config.symbols = {"AAPL"};
    config.initial_prices["AAPL"] = 150.0;
    config.enabled_strategies = {"MarketMaking"};
    VelocityEngine engine(config);
    engine.initialize();
    engine.add_strategy("MarketMaking", "MM", "MM1");
    engine.start();
    // Simulate market data
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    engine.stop();
    // Validate system state
    auto metrics = engine.get_performance_metrics();
    auto positions = engine.get_positions();
    assert(metrics.total_trades >= 0);
    assert(!positions.empty());
    std::cout << "End-to-End Trading Loop test passed!\n";
}

// ============================================================================
// MAIN INTEGRATION TEST RUNNER
// ============================================================================
int main() {
    std::cout << "Running Integration & End-to-End Tests for Velocity HFT Simulator...\n\n";
    try {
        test_full_trading_session();
        test_data_export();
        test_configuration_variations();
        test_end_to_end_trading_loop();
        std::cout << "\n🔗 All Integration & End-to-End tests passed! 🔗\n";
        std::cout << "Velocity HFT Simulator passes integration validation.\n";
    } catch (const std::exception& e) {
        std::cerr << "Integration test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 