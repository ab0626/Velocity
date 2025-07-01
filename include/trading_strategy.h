#pragma once

#include "order_manager.h"
#include <memory>
#include <vector>
#include <deque>
#include <random>

namespace velocity {

// Strategy performance metrics
struct StrategyMetrics {
    double total_pnl;
    double sharpe_ratio;
    double max_drawdown;
    double win_rate;
    uint64_t total_trades;
    uint64_t winning_trades;
    double avg_trade_duration;
    double avg_latency;
    
    StrategyMetrics() : total_pnl(0.0), sharpe_ratio(0.0), max_drawdown(0.0), 
                       win_rate(0.0), total_trades(0), winning_trades(0),
                       avg_trade_duration(0.0), avg_latency(0.0) {}
};

// Base trading strategy class
class TradingStrategy {
protected:
    std::string name_;
    std::string trader_id_;
    std::shared_ptr<OrderManager> order_manager_;
    std::vector<std::string> symbols_;
    
    // Performance tracking
    StrategyMetrics metrics_;
    std::deque<double> returns_history_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
    // Strategy state
    bool running_;
    std::mutex strategy_mutex_;
    
public:
    TradingStrategy(const std::string& name, const std::string& trader_id, 
                   std::shared_ptr<OrderManager> order_manager);
    virtual ~TradingStrategy() = default;
    
    // Strategy lifecycle
    virtual void initialize() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void on_market_data(const std::string& symbol, double bid, double ask) = 0;
    virtual void on_execution(const Execution& execution) = 0;
    
    // Configuration
    void add_symbol(const std::string& symbol);
    void set_trader_id(const std::string& trader_id);
    
    // Performance
    StrategyMetrics get_metrics() const;
    void update_metrics(const Execution& execution);
    
    // Accessors
    std::string get_name() const { return name_; }
    std::string get_trader_id() const { return trader_id_; }
    bool is_running() const { return running_; }
    
protected:
    // Utility methods for strategies
    uint64_t place_market_order(const std::string& symbol, OrderSide side, uint32_t quantity);
    uint64_t place_limit_order(const std::string& symbol, OrderSide side, double price, uint32_t quantity);
    bool cancel_order(uint64_t order_id);
    
    // Risk management
    bool check_position_limit(const std::string& symbol, int32_t additional_quantity);
    double get_position_value(const std::string& symbol);
    
    // Market data access
    double get_best_bid(const std::string& symbol);
    double get_best_ask(const std::string& symbol);
    double get_mid_price(const std::string& symbol);
    double get_spread(const std::string& symbol);
    
    // Performance calculation
    void calculate_sharpe_ratio();
    void calculate_win_rate();
    void update_drawdown(double current_pnl);
};

// Market making strategy
class MarketMakingStrategy : public TradingStrategy {
private:
    // Strategy parameters
    double spread_multiplier_;
    uint32_t base_quantity_;
    double max_position_;
    double min_spread_;
    
    // State tracking
    std::map<std::string, uint64_t> active_bid_orders_;
    std::map<std::string, uint64_t> active_ask_orders_;
    std::map<std::string, double> last_bid_prices_;
    std::map<std::string, double> last_ask_prices_;
    
public:
    MarketMakingStrategy(const std::string& name, const std::string& trader_id,
                        std::shared_ptr<OrderManager> order_manager);
    
    void initialize() override;
    void start() override;
    void stop() override;
    void on_market_data(const std::string& symbol, double bid, double ask) override;
    void on_execution(const Execution& execution) override;
    
    // Configuration
    void set_spread_multiplier(double multiplier) { spread_multiplier_ = multiplier; }
    void set_base_quantity(uint32_t quantity) { base_quantity_ = quantity; }
    void set_max_position(double max_pos) { max_position_ = max_pos; }
    void set_min_spread(double min_spread) { min_spread_ = min_spread; }
    
private:
    void update_quotes(const std::string& symbol);
    void cancel_old_quotes(const std::string& symbol);
    double calculate_bid_price(const std::string& symbol);
    double calculate_ask_price(const std::string& symbol);
    uint32_t calculate_quantity(const std::string& symbol, OrderSide side);
};

// Statistical arbitrage strategy
class StatArbStrategy : public TradingStrategy {
private:
    // Strategy parameters
    double z_score_threshold_;
    uint32_t lookback_period_;
    double position_size_;
    
    // Price history
    std::map<std::string, std::deque<double>> price_history_;
    std::map<std::string, double> mean_prices_;
    std::map<std::string, double> std_prices_;
    
    // Pairs trading state
    std::string pair_symbol1_;
    std::string pair_symbol2_;
    std::deque<double> spread_history_;
    double spread_mean_;
    double spread_std_;
    
public:
    StatArbStrategy(const std::string& name, const std::string& trader_id,
                   std::shared_ptr<OrderManager> order_manager);
    
    void initialize() override;
    void start() override;
    void stop() override;
    void on_market_data(const std::string& symbol, double bid, double ask) override;
    void on_execution(const Execution& execution) override;
    
    // Configuration
    void set_pair_symbols(const std::string& symbol1, const std::string& symbol2);
    void set_z_score_threshold(double threshold) { z_score_threshold_ = threshold; }
    void set_lookback_period(uint32_t period) { lookback_period_ = period; }
    void set_position_size(double size) { position_size_ = size; }
    
private:
    void update_price_history(const std::string& symbol, double price);
    void calculate_statistics();
    double calculate_z_score(const std::string& symbol);
    double calculate_spread_z_score();
    void execute_pairs_trade(double z_score);
    void close_positions();
};

// Momentum strategy
class MomentumStrategy : public TradingStrategy {
private:
    // Strategy parameters
    uint32_t short_window_;
    uint32_t long_window_;
    double momentum_threshold_;
    uint32_t position_size_;
    
    // Price history
    std::map<std::string, std::deque<double>> price_history_;
    std::map<std::string, double> short_ma_;
    std::map<std::string, double> long_ma_;
    
    // Position tracking
    std::map<std::string, OrderSide> current_signals_;
    
public:
    MomentumStrategy(const std::string& name, const std::string& trader_id,
                    std::shared_ptr<OrderManager> order_manager);
    
    void initialize() override;
    void start() override;
    void stop() override;
    void on_market_data(const std::string& symbol, double bid, double ask) override;
    void on_execution(const Execution& execution) override;
    
    // Configuration
    void set_windows(uint32_t short_window, uint32_t long_window);
    void set_momentum_threshold(double threshold) { momentum_threshold_ = threshold; }
    void set_position_size(uint32_t size) { position_size_ = size; }
    
private:
    void update_moving_averages(const std::string& symbol, double price);
    double calculate_momentum(const std::string& symbol);
    void generate_signals(const std::string& symbol);
    void execute_signal(const std::string& symbol, OrderSide signal);
};

// Market order strategy - places market orders periodically
class MarketOrderStrategy : public TradingStrategy {
private:
    // Strategy parameters
    uint32_t order_interval_ms_;
    uint32_t order_size_;
    uint32_t max_orders_;
    uint32_t order_count_;
    
    // Timing
    std::chrono::high_resolution_clock::time_point last_order_time_;
    
public:
    MarketOrderStrategy(const std::string& name, const std::string& trader_id,
                       std::shared_ptr<OrderManager> order_manager);
    
    void initialize() override;
    void start() override;
    void stop() override;
    void on_market_data(const std::string& symbol, double bid, double ask) override;
    void on_execution(const Execution& execution) override;
    
    // Configuration
    void set_order_interval(uint32_t interval_ms) { order_interval_ms_ = interval_ms; }
    void set_order_size(uint32_t size) { order_size_ = size; }
    void set_max_orders(uint32_t max) { max_orders_ = max; }
};

// Strategy factory
class StrategyFactory {
public:
    static std::unique_ptr<TradingStrategy> create_strategy(
        const std::string& strategy_type,
        const std::string& name,
        const std::string& trader_id,
        std::shared_ptr<OrderManager> order_manager
    );
};

} // namespace velocity 