#pragma once

#include "market_data.h"
#include <unordered_map>
#include <queue>
#include <thread>
#include <condition_variable>

namespace velocity {

// Order execution result
struct Execution {
    uint64_t order_id;
    uint64_t execution_id;
    std::string symbol;
    OrderSide side;
    double price;
    uint32_t quantity;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string trader_id;
    
    Execution() : order_id(0), execution_id(0), price(0.0), quantity(0) {}
};

// Position tracking
struct Position {
    std::string symbol;
    int32_t quantity;  // Positive for long, negative for short
    double avg_price;
    double unrealized_pnl;
    double realized_pnl;
    
    Position() : quantity(0), avg_price(0.0), unrealized_pnl(0.0), realized_pnl(0.0) {}
};

// Risk limits
struct RiskLimits {
    double max_position_value;
    double max_daily_loss;
    double max_drawdown;
    uint32_t max_order_size;
    double max_leverage;
    
    RiskLimits() : max_position_value(1000000.0), max_daily_loss(50000.0), 
                   max_drawdown(0.1), max_order_size(10000), max_leverage(2.0) {}
};

// Order matching engine
class MatchingEngine {
private:
    std::map<std::string, OrderBook> order_books_;
    mutable std::mutex engine_mutex_;
    std::queue<Order> order_queue_;
    std::condition_variable order_cv_;
    std::thread matching_thread_;
    std::atomic<bool> running_;
    
    // Execution callbacks
    std::function<void(const Execution&)> execution_callback_;
    std::function<void(const Order&)> order_status_callback_;
    
    // Statistics
    uint64_t total_orders_processed_;
    uint64_t total_executions_;
    double total_volume_;
    
public:
    MatchingEngine();
    ~MatchingEngine();
    
    // Engine control
    void start();
    void stop();
    void add_symbol(const std::string& symbol);
    
    // Order processing
    uint64_t submit_order(const Order& order);
    bool cancel_order(uint64_t order_id, const std::string& trader_id);
    bool modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, const std::string& trader_id);
    
    // Market data access
    OrderBook& get_order_book(const std::string& symbol);
    const OrderBook& get_order_book(const std::string& symbol) const;
    
    // Callbacks
    void set_execution_callback(std::function<void(const Execution&)> callback);
    void set_order_status_callback(std::function<void(const Order&)> callback);
    
    // Statistics
    uint64_t get_total_orders() const { return total_orders_processed_; }
    uint64_t get_total_executions() const { return total_executions_; }
    double get_total_volume() const { return total_volume_; }
    
private:
    void matching_loop();
    void process_order(const Order& order);
    void process_market_order(const Order& order);
    void match_orders(const std::string& symbol);
    void execute_trade(const Order& buy_order, const Order& sell_order, double price, uint32_t quantity);
    uint64_t generate_order_id();
    uint64_t generate_execution_id();
    
    std::atomic<uint64_t> order_id_counter_;
    std::atomic<uint64_t> execution_id_counter_;
};

// Order management system
class OrderManager {
private:
    MatchingEngine matching_engine_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, Order>> active_orders_;
    std::unordered_map<std::string, Position> positions_;
    mutable std::mutex manager_mutex_;
    
    // Risk management
    RiskLimits risk_limits_;
    double daily_pnl_;
    double max_drawdown_;
    double peak_equity_;
    
    // Callbacks
    std::function<void(const Execution&)> execution_callback_;
    std::function<void(const Position&)> position_callback_;
    std::function<void(const std::string&)> risk_alert_callback_;
    
public:
    OrderManager();
    
    // Add symbol
    void add_symbol(const std::string& symbol);
    
    // Order management
    uint64_t place_order(const Order& order);
    bool cancel_order(uint64_t order_id, const std::string& trader_id);
    bool modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, const std::string& trader_id);
    
    // Position management
    Position get_position(const std::string& symbol) const;
    std::vector<Position> get_all_positions() const;
    void update_position(const Execution& execution);
    
    // Risk management
    bool check_risk_limits(const Order& order);
    void set_risk_limits(const RiskLimits& limits);
    RiskLimits get_risk_limits() const { return risk_limits_; }
    
    // P&L tracking
    double get_daily_pnl() const { return daily_pnl_; }
    double get_max_drawdown() const { return max_drawdown_; }
    double get_total_pnl() const;
    
    // Callbacks
    void set_execution_callback(std::function<void(const Execution&)> callback);
    void set_position_callback(std::function<void(const Position&)> callback);
    void set_risk_alert_callback(std::function<void(const std::string&)> callback);
    
    // Market data access
    OrderBook& get_order_book(const std::string& symbol);
    const OrderBook& get_order_book(const std::string& symbol) const;
    
private:
    void on_execution(const Execution& execution);
    void on_order_status(const Order& order);
    bool check_position_limits(const Order& order);
    bool check_daily_loss_limit(const Order& order);
    void update_drawdown();
    void send_risk_alert(const std::string& message);
};

} // namespace velocity 