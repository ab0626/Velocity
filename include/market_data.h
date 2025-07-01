#pragma once

#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>

namespace velocity {

// Market data types
enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    PENDING,
    PARTIAL,
    FILLED,
    CANCELLED,
    REJECTED
};

// Order structure
struct Order {
    uint64_t id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    double price;
    uint32_t quantity;
    uint32_t filled_quantity;
    OrderStatus status;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string trader_id;
    
    Order() : id(0), price(0.0), quantity(0), filled_quantity(0), status(OrderStatus::PENDING) {}
};

// Price level in order book
struct PriceLevel {
    double price;
    uint32_t total_quantity;
    std::vector<Order> orders;
    
    PriceLevel(double p = 0.0) : price(p), total_quantity(0) {}
};

// Order book for a single symbol
class OrderBook {
private:
    std::string symbol_;
    std::map<double, PriceLevel, std::greater<double>> bids_; // Descending order for bids
    std::map<double, PriceLevel> asks_; // Ascending order for asks
    mutable std::mutex book_mutex_;
    
    double last_price_;
    double best_bid_;
    double best_ask_;
    uint64_t sequence_number_;
    
public:
    OrderBook();
    OrderBook(const std::string& symbol);
    OrderBook(const OrderBook& other);
    OrderBook(OrderBook&& other) noexcept;
    OrderBook& operator=(const OrderBook& other);
    OrderBook& operator=(OrderBook&& other) noexcept;
    std::string get_symbol() const;
    
    // Order book operations
    void add_order(const Order& order);
    void cancel_order(uint64_t order_id);
    void modify_order(uint64_t order_id, double new_price, uint32_t new_quantity);
    
    // Market data queries
    double get_best_bid() const;
    double get_best_ask() const;
    double get_mid_price() const;
    double get_spread() const;
    double get_last_price() const;
    
    // Order book depth
    std::vector<PriceLevel> get_bid_levels(uint32_t depth = 10) const;
    std::vector<PriceLevel> get_ask_levels(uint32_t depth = 10) const;
    
    // Market data callbacks
    void set_price_update_callback(std::function<void(const std::string&, double, double)> callback);
    
    void set_last_price(double price);
    
    // Simulation/test utilities
    void clear_book();
    void add_limit_order(double price, uint32_t quantity, OrderSide side);
    
private:
    void update_best_prices();
    void notify_price_update();
    
    std::function<void(const std::string&, double, double)> price_callback_;
};

// Market data feed simulator
class MarketDataFeed {
private:
    std::map<std::string, OrderBook> order_books_;
    mutable std::mutex feed_mutex_;
    std::atomic<bool> running_;
    std::thread feed_thread_;
    
    // Market data callbacks
    std::function<void(const std::string&, double, double)> price_callback_;
    std::function<void(const Order&)> order_callback_;
    
public:
    MarketDataFeed();
    ~MarketDataFeed();
    
    // Feed management
    void start();
    void stop();
    void add_symbol(const std::string& symbol, double initial_price);
    void update_price(const std::string& symbol, double price);
    
    // Order book access
    OrderBook& get_order_book(const std::string& symbol);
    const OrderBook& get_order_book(const std::string& symbol) const;
    
    // Callbacks
    void set_price_callback(std::function<void(const std::string&, double, double)> callback);
    void set_order_callback(std::function<void(const Order&)> callback);
    
private:
    void feed_loop();
    void generate_market_data();
    void simulate_market_microstructure();
};

} // namespace velocity 