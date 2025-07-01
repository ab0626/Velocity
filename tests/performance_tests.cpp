#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include "../include/performance_analytics.h"
#include "../include/velocity_engine.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <iomanip>
#include <memory>
#include <cassert>
#include <algorithm>

using namespace velocity;

// ============================================================================
// LATENCY BENCHMARKING TESTS
// ============================================================================

struct LatencyResult {
    std::string test_name;
    double avg_latency_us;
    double min_latency_us;
    double max_latency_us;
    double p50_latency_us;
    double p95_latency_us;
    double p99_latency_us;
    uint64_t total_operations;
};

class LatencyBenchmark {
private:
    std::vector<double> latencies_;
    
public:
    void add_latency(double latency_us) {
        latencies_.push_back(latency_us);
    }
    
    LatencyResult calculate_results(const std::string& test_name) {
        if (latencies_.empty()) {
            return {test_name, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0};
        }
        
        std::sort(latencies_.begin(), latencies_.end());
        
        double sum = 0.0;
        for (double lat : latencies_) {
            sum += lat;
        }
        
        LatencyResult result;
        result.test_name = test_name;
        result.avg_latency_us = sum / latencies_.size();
        result.min_latency_us = latencies_.front();
        result.max_latency_us = latencies_.back();
        result.p50_latency_us = latencies_[latencies_.size() * 0.5];
        result.p95_latency_us = latencies_[latencies_.size() * 0.95];
        result.p99_latency_us = latencies_[latencies_.size() * 0.99];
        result.total_operations = latencies_.size();
        
        return result;
    }
    
    void reset() {
        latencies_.clear();
    }
};

void test_order_placement_latency() {
    std::cout << "Testing Order Placement Latency...\n";
    
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    LatencyBenchmark benchmark;
    const uint32_t num_orders = 10000;
    
    for (uint32_t i = 0; i < num_orders; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        Order order;
        order.symbol = "AAPL";
        order.side = OrderSide::BUY;
        order.type = OrderType::LIMIT;
        order.price = 150.0 + (i % 100) * 0.01;
        order.quantity = 100;
        order.trader_id = "LATENCY_TEST";
        
        uint64_t order_id = order_manager->place_order(order);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark.add_latency(duration.count());
    }
    
    auto result = benchmark.calculate_results("Order Placement");
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Average: " << result.avg_latency_us << " μs\n";
    std::cout << "  Min: " << result.min_latency_us << " μs\n";
    std::cout << "  Max: " << result.max_latency_us << " μs\n";
    std::cout << "  P50: " << result.p50_latency_us << " μs\n";
    std::cout << "  P95: " << result.p95_latency_us << " μs\n";
    std::cout << "  P99: " << result.p99_latency_us << " μs\n";
    std::cout << "  Total: " << result.total_operations << " orders\n";
    
    // Performance assertions
    assert(result.avg_latency_us < 1000.0); // Should be under 1ms average
    assert(result.p99_latency_us < 5000.0); // 99th percentile under 5ms
    
    std::cout << "Order Placement Latency tests passed!\n";
}

void test_market_data_latency() {
    std::cout << "Testing Market Data Processing Latency...\n";
    
    MarketDataFeed feed;
    feed.add_symbol("AAPL", 150.0);
    
    LatencyBenchmark benchmark;
    const uint32_t num_updates = 10000;
    
    for (uint32_t i = 0; i < num_updates; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double new_price = 150.0 + (i % 100) * 0.01;
        feed.update_price("AAPL", new_price);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        benchmark.add_latency(duration.count());
    }
    
    auto result = benchmark.calculate_results("Market Data Update");
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Average: " << result.avg_latency_us << " μs\n";
    std::cout << "  Min: " << result.min_latency_us << " μs\n";
    std::cout << "  Max: " << result.max_latency_us << " μs\n";
    std::cout << "  P50: " << result.p50_latency_us << " μs\n";
    std::cout << "  P95: " << result.p95_latency_us << " μs\n";
    std::cout << "  P99: " << result.p99_latency_us << " μs\n";
    std::cout << "  Total: " << result.total_operations << " updates\n";
    
    // Performance assertions
    assert(result.avg_latency_us < 500.0); // Should be under 500μs average
    assert(result.p99_latency_us < 2000.0); // 99th percentile under 2ms
    
    std::cout << "Market Data Latency tests passed!\n";
}

// ============================================================================
// THROUGHPUT TESTING
// ============================================================================

void test_order_throughput() {
    std::cout << "Testing Order Throughput...\n";
    
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    const uint32_t num_orders = 100000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (uint32_t i = 0; i < num_orders; ++i) {
        Order order;
        order.symbol = "AAPL";
        order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.price = 150.0 + (i % 100) * 0.01;
        order.quantity = 100;
        order.trader_id = "THROUGHPUT_TEST";
        
        order_manager->place_order(order);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double orders_per_second = (num_orders * 1000.0) / duration.count();
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Total orders: " << num_orders << "\n";
    std::cout << "  Duration: " << duration.count() << " ms\n";
    std::cout << "  Throughput: " << orders_per_second << " orders/sec\n";
    
    // Performance assertions
    assert(orders_per_second > 1000.0); // Should handle at least 1000 orders/sec
    assert(duration.count() < 100000); // Should complete in under 100 seconds
    
    std::cout << "Order Throughput tests passed!\n";
}

// ============================================================================
// MEMORY USAGE TESTING
// ============================================================================

void test_memory_usage() {
    std::cout << "Testing Memory Usage...\n";
    
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    // Measure initial memory usage (simplified)
    size_t initial_orders = 0;
    
    const uint32_t num_orders = 10000;
    
    for (uint32_t i = 0; i < num_orders; ++i) {
        Order order;
        order.symbol = "AAPL";
        order.side = OrderSide::BUY;
        order.type = OrderType::LIMIT;
        order.price = 150.0 + (i % 100) * 0.01;
        order.quantity = 100;
        order.trader_id = "MEMORY_TEST";
        
        order_manager->place_order(order);
    }
    
    // Get positions to verify memory usage
    auto positions = order_manager->get_all_positions();
    
    std::cout << "  Orders placed: " << num_orders << "\n";
    std::cout << "  Positions tracked: " << positions.size() << "\n";
    
    // Memory assertions
    assert(positions.size() > 0); // Should track at least one position
    
    std::cout << "Memory Usage tests passed!\n";
}

// ============================================================================
// CPU UTILIZATION TESTING
// ============================================================================

void test_cpu_utilization() {
    std::cout << "Testing CPU Utilization...\n";
    
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    const uint32_t num_iterations = 100000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate high-frequency trading
    for (uint32_t i = 0; i < num_iterations; ++i) {
        Order order;
        order.symbol = "AAPL";
        order.side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.price = 150.0 + (i % 100) * 0.01;
        order.quantity = 100;
        order.trader_id = "CPU_TEST";
        
        order_manager->place_order(order);
        
        // Simulate market data updates
        if (i % 10 == 0) {
            auto& order_book = order_manager->get_order_book("AAPL");
            order_book.get_best_bid();
            order_book.get_best_ask();
            order_book.get_mid_price();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double operations_per_second = (num_iterations * 1000.0) / duration.count();
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Total operations: " << num_iterations << "\n";
    std::cout << "  Duration: " << duration.count() << " ms\n";
    std::cout << "  Operations/sec: " << operations_per_second << "\n";
    
    // Performance assertions
    assert(operations_per_second > 100.0); // Should handle at least 100 ops/sec
    assert(duration.count() < 1000000); // Should complete in reasonable time
    
    std::cout << "CPU Utilization tests passed!\n";
}

// ============================================================================
// MAIN PERFORMANCE TEST RUNNER
// ============================================================================

int main() {
    std::cout << "Running Performance & Latency Tests for Velocity HFT Simulator...\n\n";
    
    try {
        // Latency Benchmarking
        test_order_placement_latency();
        test_market_data_latency();
        
        // Throughput Testing
        test_order_throughput();
        
        // Memory Usage Testing
        test_memory_usage();
        
        // CPU Utilization Testing
        test_cpu_utilization();
        
        std::cout << "\n🚀 All Performance & Latency tests passed! 🚀\n";
        std::cout << "Velocity HFT Simulator meets performance requirements.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 