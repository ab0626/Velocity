#pragma once

#include "market_data.h"
#include "order_manager.h"
#include "trading_strategy.h"
#include "performance_analytics.h"
#include <memory>
#include <vector>
#include <map>
#include <thread>
#include <atomic>

namespace velocity {

// Configuration for the Velocity engine
struct VelocityConfig {
    // Market data settings
    std::vector<std::string> symbols;
    std::map<std::string, double> initial_prices;
    double volatility_multiplier;
    uint32_t market_data_frequency_ms;
    
    // Order management settings
    uint32_t max_order_size;
    double max_position_value;
    double max_daily_loss;
    double max_drawdown;
    
    // Strategy settings
    std::vector<std::string> enabled_strategies;
    std::map<std::string, std::map<std::string, std::string>> strategy_params;
    
    // Performance settings
    bool enable_logging;
    std::string log_directory;
    uint32_t performance_update_frequency_ms;
    
    // Web dashboard settings
    bool enable_dashboard;
    uint16_t dashboard_port;
    
    VelocityConfig() : volatility_multiplier(1.0), market_data_frequency_ms(100),
                      max_order_size(10000), max_position_value(1000000.0),
                      max_daily_loss(50000.0), max_drawdown(0.1),
                      enable_logging(true), log_directory("./logs"),
                      performance_update_frequency_ms(1000),
                      enable_dashboard(true), dashboard_port(8080) {}
};

// Main Velocity HFT engine
class VelocityEngine {
private:
    // Core components
    std::shared_ptr<MarketDataFeed> market_data_feed_;
    std::shared_ptr<OrderManager> order_manager_;
    std::shared_ptr<PerformanceAnalytics> analytics_;
    std::shared_ptr<PerformanceMonitor> monitor_;
    std::shared_ptr<DashboardDataProvider> dashboard_provider_;
    
    // Strategy management
    std::vector<std::shared_ptr<TradingStrategy>> strategies_;
    std::map<std::string, std::shared_ptr<TradingStrategy>> strategy_map_;
    
    // Engine state
    std::atomic<bool> running_;
    std::thread engine_thread_;
    VelocityConfig config_;
    
    // Web dashboard
    std::thread dashboard_thread_;
    std::atomic<bool> dashboard_running_;
    
public:
    VelocityEngine(const VelocityConfig& config = VelocityConfig());
    ~VelocityEngine();
    
    // Engine lifecycle
    void initialize();
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Configuration
    void set_config(const VelocityConfig& config);
    VelocityConfig get_config() const { return config_; }
    
    // Strategy management
    void add_strategy(const std::string& strategy_type, const std::string& name, 
                     const std::string& trader_id, 
                     const std::map<std::string, std::string>& params = {});
    void remove_strategy(const std::string& name);
    void start_strategy(const std::string& name);
    void stop_strategy(const std::string& name);
    std::vector<std::string> get_strategy_names() const;
    
    // Market data management
    void add_symbol(const std::string& symbol, double initial_price);
    void remove_symbol(const std::string& symbol);
    std::vector<std::string> get_symbols() const;
    
    // Order management
    uint64_t place_order(const Order& order);
    bool cancel_order(uint64_t order_id, const std::string& trader_id);
    bool modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, 
                     const std::string& trader_id);
    
    // Performance and analytics
    PerformanceMetrics get_performance_metrics() const;
    RiskMetrics get_risk_metrics() const;
    std::vector<Position> get_positions() const;
    DashboardData get_dashboard_data() const;
    
    // Market data access
    OrderBook& get_order_book(const std::string& symbol);
    const OrderBook& get_order_book(const std::string& symbol) const;
    
    // Logging and export
    void export_trades_to_csv(const std::string& filename);
    void export_performance_to_csv(const std::string& filename);
    void enable_logging(const std::string& directory);
    
    // Web dashboard
    void start_dashboard(uint16_t port = 8080);
    void stop_dashboard();
    bool is_dashboard_running() const { return dashboard_running_; }
    
    std::shared_ptr<OrderManager> get_order_manager();
    
private:
    // Internal methods
    void engine_loop();
    void initialize_market_data();
    void initialize_strategies();
    void initialize_analytics();
    void setup_callbacks();
    
    // Strategy factory
    std::shared_ptr<TradingStrategy> create_strategy(const std::string& strategy_type,
                                                    const std::string& name,
                                                    const std::string& trader_id,
                                                    const std::map<std::string, std::string>& params);
    
    // Dashboard server
    void dashboard_server_loop(uint16_t port);
    std::string generate_dashboard_html() const;
    std::string generate_dashboard_json() const;
    
    // Utility methods
    void log_engine_event(const std::string& event);
    void handle_error(const std::string& error);
};

// Velocity engine builder for easy configuration
class VelocityEngineBuilder {
private:
    VelocityConfig config_;
    
public:
    VelocityEngineBuilder();
    
    // Market data configuration
    VelocityEngineBuilder& add_symbol(const std::string& symbol, double initial_price);
    VelocityEngineBuilder& set_volatility(double multiplier);
    VelocityEngineBuilder& set_market_data_frequency(uint32_t ms);
    
    // Risk management configuration
    VelocityEngineBuilder& set_max_order_size(uint32_t size);
    VelocityEngineBuilder& set_max_position_value(double value);
    VelocityEngineBuilder& set_max_daily_loss(double loss);
    VelocityEngineBuilder& set_max_drawdown(double drawdown);
    
    // Strategy configuration
    VelocityEngineBuilder& add_strategy(const std::string& strategy_type, 
                                       const std::map<std::string, std::string>& params = {});
    VelocityEngineBuilder& set_strategy_param(const std::string& strategy, 
                                             const std::string& param, 
                                             const std::string& value);
    
    // Performance configuration
    VelocityEngineBuilder& enable_logging(const std::string& directory = "./logs");
    VelocityEngineBuilder& set_performance_update_frequency(uint32_t ms);
    
    // Dashboard configuration
    VelocityEngineBuilder& enable_dashboard(uint16_t port = 8080);
    VelocityEngineBuilder& disable_dashboard();
    
    // Build the engine
    std::unique_ptr<VelocityEngine> build();
};

// Utility functions
namespace utils {
    // Time utilities
    std::string get_current_timestamp();
    uint64_t get_current_time_microseconds();
    
    // Math utilities
    double calculate_std_dev(const std::vector<double>& values);
    double calculate_mean(const std::vector<double>& values);
    double calculate_percentile(const std::vector<double>& values, double percentile);
    
    // String utilities
    std::string format_currency(double value);
    std::string format_percentage(double value);
    std::string format_number(double value, int precision = 2);
    
    // File utilities
    bool create_directory(const std::string& path);
    std::string get_file_extension(const std::string& filename);
    bool file_exists(const std::string& filename);
}

} // namespace velocity 