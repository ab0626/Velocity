#pragma once

#include "order_manager.h"
#include "trading_strategy.h"
#include <vector>
#include <deque>
#include <map>
#include <chrono>
#include <fstream>

namespace velocity {

// Performance metrics
struct PerformanceMetrics {
    double total_pnl;
    double realized_pnl;
    double unrealized_pnl;
    double sharpe_ratio;
    double sortino_ratio;
    double max_drawdown;
    double win_rate;
    double profit_factor;
    double avg_win;
    double avg_loss;
    double largest_win;
    double largest_loss;
    uint64_t total_trades;
    uint64_t winning_trades;
    uint64_t losing_trades;
    double avg_trade_duration;
    double avg_latency;
    double max_latency;
    double min_latency;
    
    PerformanceMetrics() : total_pnl(0.0), realized_pnl(0.0), unrealized_pnl(0.0),
                          sharpe_ratio(0.0), sortino_ratio(0.0), max_drawdown(0.0),
                          win_rate(0.0), profit_factor(0.0), avg_win(0.0), avg_loss(0.0),
                          largest_win(0.0), largest_loss(0.0), total_trades(0),
                          winning_trades(0), losing_trades(0), avg_trade_duration(0.0),
                          avg_latency(0.0), max_latency(0.0), min_latency(0.0) {}
};

// Trade record
struct Trade {
    uint64_t trade_id;
    std::string symbol;
    OrderSide side;
    double entry_price;
    double exit_price;
    uint32_t quantity;
    double pnl;
    std::chrono::high_resolution_clock::time_point entry_time;
    std::chrono::high_resolution_clock::time_point exit_time;
    std::chrono::microseconds latency;
    
    Trade() : trade_id(0), entry_price(0.0), exit_price(0.0), quantity(0), pnl(0.0) {}
};

// Latency measurement
struct LatencyMeasurement {
    uint64_t order_id;
    std::chrono::high_resolution_clock::time_point order_time;
    std::chrono::high_resolution_clock::time_point execution_time;
    std::chrono::microseconds latency;
    std::string symbol;
    OrderSide side;
    
    LatencyMeasurement() : order_id(0) {}
};

// Enhanced analysis structures
struct PnLHistogram {
    std::vector<double> bins;
    std::vector<int> frequencies;
    double min_pnl, max_pnl;
    double bin_width;
    
    PnLHistogram() : min_pnl(0), max_pnl(0), bin_width(0) {}
};

struct RiskMetrics {
    double var_95;           // 95% Value at Risk
    double var_99;           // 99% Value at Risk
    double sharpe_ratio;     // Sharpe ratio (assuming risk-free rate = 0)
    double max_drawdown;     // Maximum drawdown
    double volatility;       // PnL volatility
    double skewness;         // PnL distribution skewness
    double kurtosis;         // PnL distribution kurtosis
    double exposure;         // Current market exposure
    
    RiskMetrics() : var_95(0), var_99(0), sharpe_ratio(0), max_drawdown(0), 
                   volatility(0), skewness(0), kurtosis(0), exposure(0) {}
};

struct TradeLog {
    uint64_t trade_id;
    std::string symbol;
    OrderSide side;
    double price;
    uint32_t quantity;
    double pnl;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string trader_id;
    std::string strategy;
    
    TradeLog() : trade_id(0), price(0), quantity(0), pnl(0) {}
};

struct OrderBookSnapshot {
    std::string symbol;
    std::chrono::high_resolution_clock::time_point timestamp;
    double best_bid, best_ask, mid_price, spread;
    std::vector<PriceLevel> bid_levels;
    std::vector<PriceLevel> ask_levels;
    
    OrderBookSnapshot() : best_bid(0), best_ask(0), mid_price(0), spread(0) {}
};

// Performance analytics engine
class PerformanceAnalytics {
private:
    std::vector<Trade> trades_;
    std::vector<LatencyMeasurement> latency_measurements_;
    std::deque<double> returns_history_;
    std::deque<double> equity_curve_;
    std::map<std::string, std::vector<double>> symbol_returns_;
    
    // Performance tracking
    PerformanceMetrics metrics_;
    RiskMetrics risk_metrics_;
    
    // Configuration
    uint32_t lookback_period_;
    double risk_free_rate_;
    std::string benchmark_symbol_;
    
    // File output
    std::ofstream trade_log_;
    std::ofstream performance_log_;
    
    // Enhanced data storage
    std::vector<double> pnl_history_;
    std::vector<TradeLog> trade_logs_;
    std::vector<OrderBookSnapshot> book_snapshots_;
    std::vector<std::chrono::high_resolution_clock::time_point> timestamps_;
    
    // Risk management
    double max_drawdown_limit_;
    double max_exposure_limit_;
    bool detailed_logging_enabled_;
    
public:
    PerformanceAnalytics();
    ~PerformanceAnalytics();
    
    // Core analytics
    void record_trade(const Trade& trade);
    void record_execution(const Execution& execution);
    void update_price(const std::string& symbol, double price);
    void update_position(const std::string& symbol, int32_t quantity, double avg_price);
    
    // Enhanced analysis methods
    PnLHistogram get_pnl_histogram(int num_bins = 20) const;
    RiskMetrics calculate_risk_metrics() const;
    std::vector<TradeLog> get_trade_logs() const;
    std::vector<OrderBookSnapshot> get_order_book_snapshots() const;
    
    // Real-time monitoring
    void capture_order_book_snapshot(const std::string& symbol, const OrderBook& book);
    void enable_detailed_logging(bool enable = true);
    void set_risk_limits(double max_drawdown, double max_exposure);
    
    // Export and reporting
    void export_risk_report(const std::string& filename) const;
    void export_trade_analysis(const std::string& filename) const;
    void print_performance_summary() const;
    
    // Performance calculation
    void update_equity_curve(double current_equity);
    
    // Reporting
    PerformanceMetrics get_performance_metrics() const { return metrics_; }
    RiskMetrics get_risk_metrics() const { return risk_metrics_; }
    std::vector<Trade> get_trades() const { return trades_; }
    std::vector<LatencyMeasurement> get_latency_measurements() const { return latency_measurements_; }
    
    // Configuration
    void set_lookback_period(uint32_t period) { lookback_period_ = period; }
    void set_risk_free_rate(double rate) { risk_free_rate_ = rate; }
    void set_benchmark_symbol(const std::string& symbol) { benchmark_symbol_ = symbol; }
    
    // File output
    void enable_trade_logging(const std::string& filename);
    void enable_performance_logging(const std::string& filename);
    void export_trades_to_csv(const std::string& filename);
    void export_performance_to_csv(const std::string& filename);
    
private:
    // Calculation helpers
    double calculate_sharpe_ratio();
    double calculate_sortino_ratio();
    double calculate_max_drawdown();
    double calculate_win_rate();
    double calculate_profit_factor();
    double calculate_var(double confidence_level);
    double calculate_cvar(double confidence_level);
    double calculate_volatility();
    double calculate_beta();
    
    // Utility methods
    std::vector<double> calculate_returns();
    void update_trade_statistics();
    void update_latency_statistics();
    void log_trade(const Trade& trade);
    void log_performance();
    
    // Analysis helpers
    double calculate_var(const std::vector<double>& returns, double confidence) const;
    double calculate_sharpe_ratio(const std::vector<double>& returns) const;
    double calculate_skewness(const std::vector<double>& values) const;
    double calculate_kurtosis(const std::vector<double>& values) const;
    double calculate_max_drawdown(const std::vector<double>& equity_curve) const;
};

// Real-time performance monitor
class PerformanceMonitor {
private:
    std::shared_ptr<PerformanceAnalytics> analytics_;
    std::shared_ptr<OrderManager> order_manager_;
    std::vector<std::shared_ptr<TradingStrategy>> strategies_;
    
    std::thread monitor_thread_;
    std::atomic<bool> running_;
    std::chrono::milliseconds update_interval_;
    
    // Real-time metrics
    std::map<std::string, double> current_prices_;
    std::map<std::string, Position> current_positions_;
    double current_equity_;
    double current_pnl_;
    
public:
    PerformanceMonitor(std::shared_ptr<PerformanceAnalytics> analytics,
                      std::shared_ptr<OrderManager> order_manager);
    ~PerformanceMonitor();
    
    // Monitor control
    void start();
    void stop();
    void add_strategy(std::shared_ptr<TradingStrategy> strategy);
    
    // Real-time data
    void update_price(const std::string& symbol, double price);
    void update_position(const std::string& symbol, const Position& position);
    
    // Accessors
    double get_current_equity() const { return current_equity_; }
    double get_current_pnl() const { return current_pnl_; }
    std::map<std::string, Position> get_current_positions() const { return current_positions_; }
    
private:
    void monitor_loop();
    void update_equity();
    void update_unrealized_pnl();
};

// Performance dashboard data
struct DashboardData {
    PerformanceMetrics performance;
    RiskMetrics risk;
    std::map<std::string, Position> positions;
    std::map<std::string, double> prices;
    std::vector<double> equity_curve;
    std::vector<double> drawdown_curve;
    double current_equity;
    double current_pnl;
    uint64_t total_orders;
    uint64_t total_executions;
    double total_volume;
    
    DashboardData() : current_equity(0.0), current_pnl(0.0), 
                     total_orders(0), total_executions(0), total_volume(0.0) {}
};

// Dashboard data provider
class DashboardDataProvider {
private:
    std::shared_ptr<PerformanceAnalytics> analytics_;
    std::shared_ptr<OrderManager> order_manager_;
    std::shared_ptr<PerformanceMonitor> monitor_;
    
public:
    DashboardDataProvider(std::shared_ptr<PerformanceAnalytics> analytics,
                         std::shared_ptr<OrderManager> order_manager,
                         std::shared_ptr<PerformanceMonitor> monitor);
    
    DashboardData get_dashboard_data() const;
    std::vector<double> get_equity_curve(uint32_t points = 100) const;
    std::vector<double> get_drawdown_curve(uint32_t points = 100) const;
    std::vector<Trade> get_recent_trades(uint32_t count = 50) const;
    std::vector<LatencyMeasurement> get_recent_latencies(uint32_t count = 50) const;
};

} // namespace velocity 