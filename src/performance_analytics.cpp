#include "../include/performance_analytics.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <string>

namespace velocity {

// Utility functions
static double calculate_mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}
static double calculate_std_dev(const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double mean = calculate_mean(v);
    double sum = 0.0;
    for (double x : v) sum += (x - mean) * (x - mean);
    return std::sqrt(sum / (v.size() - 1));
}

// PerformanceAnalytics implementation
PerformanceAnalytics::PerformanceAnalytics() 
    : lookback_period_(252), risk_free_rate_(0.02) {
}

PerformanceAnalytics::~PerformanceAnalytics() {
    if (trade_log_.is_open()) {
        trade_log_.close();
    }
    if (performance_log_.is_open()) {
        performance_log_.close();
    }
}

void PerformanceAnalytics::record_trade(const Trade& trade) {
    trades_.push_back(trade);
    
    // Calculate return
    double return_val = trade.pnl / (trade.entry_price * trade.quantity);
    returns_history_.push_back(return_val);
    
    // Keep only recent returns
    if (returns_history_.size() > lookback_period_) {
        returns_history_.pop_front();
    }
    
    if (trade_log_.is_open()) {
        log_trade(trade);
    }
}

void PerformanceAnalytics::update_position(const std::string& symbol, int32_t quantity, double current_price) {
    // Update unrealized P&L for positions
    for (auto& trade : trades_) {
        if (trade.symbol == symbol && trade.exit_price == 0) {
            // This is an open position
            trade.exit_price = current_price;
            trade.pnl = (trade.exit_price - trade.entry_price) * trade.quantity;
            if (trade.side == OrderSide::SELL) {
                trade.pnl = -trade.pnl;
            }
        }
    }
}

void PerformanceAnalytics::update_price(const std::string& symbol, double price) {
    // Update price for PnL tracking
    if (pnl_history_.empty()) {
        pnl_history_.push_back(0.0);
    } else {
        // Calculate PnL change based on price movement
        double pnl_change = 0.0;
        for (const auto& trade : trades_) {
            if (trade.symbol == symbol && trade.exit_price == 0) {
                // Open position - calculate unrealized PnL
                double unrealized_pnl = (price - trade.entry_price) * trade.quantity;
                if (trade.side == OrderSide::SELL) {
                    unrealized_pnl = -unrealized_pnl;
                }
                pnl_change += unrealized_pnl;
            }
        }
        pnl_history_.push_back(pnl_history_.back() + pnl_change);
    }
    
    // Keep only recent data
    if (pnl_history_.size() > 1000) {
        pnl_history_.erase(pnl_history_.begin());
    }
    
    // Update timestamps
    timestamps_.push_back(std::chrono::high_resolution_clock::now());
    if (timestamps_.size() > 1000) {
        timestamps_.erase(timestamps_.begin());
    }
}

void PerformanceAnalytics::update_equity_curve(double current_equity) {
    equity_curve_.push_back(current_equity);
    
    // Keep only recent data
    if (equity_curve_.size() > 1000) {
        equity_curve_.pop_front();
    }
}

void PerformanceAnalytics::enable_trade_logging(const std::string& filename) {
    trade_log_.open(filename);
    if (trade_log_.is_open()) {
        trade_log_ << "TradeID,Symbol,Side,EntryPrice,ExitPrice,Quantity,PnL,EntryTime,ExitTime,Latency\n";
    }
}

void PerformanceAnalytics::enable_performance_logging(const std::string& filename) {
    performance_log_.open(filename);
}

void PerformanceAnalytics::export_trades_to_csv(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "TradeID,Symbol,Side,EntryPrice,ExitPrice,Quantity,PnL,EntryTime,ExitTime,Latency\n";
        
        for (const auto& trade : trades_) {
            file << trade.trade_id << ","
                 << trade.symbol << ","
                 << (trade.side == OrderSide::BUY ? "BUY" : "SELL") << ","
                 << trade.entry_price << ","
                 << trade.exit_price << ","
                 << trade.quantity << ","
                 << trade.pnl << ","
                 << trade.entry_time.time_since_epoch().count() << ","
                 << trade.exit_time.time_since_epoch().count() << ","
                 << trade.latency.count() << "\n";
        }
    }
}

void PerformanceAnalytics::export_performance_to_csv(const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Metric,Value\n";
        file << "TotalPnL," << metrics_.total_pnl << "\n";
        file << "SharpeRatio," << metrics_.sharpe_ratio << "\n";
        file << "MaxDrawdown," << metrics_.max_drawdown << "\n";
        file << "WinRate," << metrics_.win_rate << "\n";
        file << "TotalTrades," << metrics_.total_trades << "\n";
        file << "AvgLatency," << metrics_.avg_latency << "\n";
    }
}

double PerformanceAnalytics::calculate_sharpe_ratio() {
    if (returns_history_.size() < 2) return 0.0;
    
    double mean_return = calculate_mean(std::vector<double>(returns_history_.begin(), returns_history_.end()));
    double std_dev = calculate_std_dev(std::vector<double>(returns_history_.begin(), returns_history_.end()));
    
    if (std_dev == 0) return 0.0;
    
    return (mean_return - risk_free_rate_) / std_dev;
}

double PerformanceAnalytics::calculate_sortino_ratio() {
    if (returns_history_.size() < 2) return 0.0;
    
    double mean_return = calculate_mean(std::vector<double>(returns_history_.begin(), returns_history_.end()));
    
    // Calculate downside deviation
    double downside_sum = 0.0;
    int downside_count = 0;
    
    for (double ret : returns_history_) {
        if (ret < mean_return) {
            downside_sum += (ret - mean_return) * (ret - mean_return);
            downside_count++;
        }
    }
    
    if (downside_count == 0) return 0.0;
    
    double downside_deviation = std::sqrt(downside_sum / downside_count);
    if (downside_deviation == 0) return 0.0;
    
    return (mean_return - risk_free_rate_) / downside_deviation;
}

double PerformanceAnalytics::calculate_max_drawdown() {
    if (equity_curve_.empty()) return 0.0;
    
    double max_drawdown = 0.0;
    double peak = equity_curve_[0];
    
    for (double equity : equity_curve_) {
        if (equity > peak) {
            peak = equity;
        }
        
        double drawdown = (peak - equity) / peak;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }
    
    return max_drawdown;
}

double PerformanceAnalytics::calculate_win_rate() {
    if (trades_.empty()) return 0.0;
    
    int winning_trades = 0;
    for (const auto& trade : trades_) {
        if (trade.pnl > 0) {
            winning_trades++;
        }
    }
    
    return static_cast<double>(winning_trades) / trades_.size();
}

double PerformanceAnalytics::calculate_profit_factor() {
    double gross_profit = 0.0;
    double gross_loss = 0.0;
    
    for (const auto& trade : trades_) {
        if (trade.pnl > 0) {
            gross_profit += trade.pnl;
        } else {
            gross_loss += std::abs(trade.pnl);
        }
    }
    
    if (gross_loss == 0) return 0.0;
    return gross_profit / gross_loss;
}

double PerformanceAnalytics::calculate_var(double confidence_level) {
    if (returns_history_.size() < 2) return 0.0;
    
    std::vector<double> sorted_returns(returns_history_.begin(), returns_history_.end());
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    int index = static_cast<int>((1.0 - confidence_level) * sorted_returns.size());
    if (index >= sorted_returns.size()) index = sorted_returns.size() - 1;
    
    return sorted_returns[index];
}

double PerformanceAnalytics::calculate_cvar(double confidence_level) {
    if (returns_history_.size() < 2) return 0.0;
    
    double var = calculate_var(confidence_level);
    
    double sum = 0.0;
    int count = 0;
    
    for (double ret : returns_history_) {
        if (ret <= var) {
            sum += ret;
            count++;
        }
    }
    
    if (count == 0) return var;
    return sum / count;
}

double PerformanceAnalytics::calculate_volatility() {
    return calculate_std_dev(std::vector<double>(returns_history_.begin(), returns_history_.end()));
}

double PerformanceAnalytics::calculate_beta() {
    // Simplified beta calculation (would need benchmark data in real implementation)
    return 1.0;
}

std::vector<double> PerformanceAnalytics::calculate_returns() {
    return std::vector<double>(returns_history_.begin(), returns_history_.end());
}

void PerformanceAnalytics::update_trade_statistics() {
    if (trades_.empty()) return;
    
    double total_pnl = 0.0;
    double total_wins = 0.0;
    double total_losses = 0.0;
    double largest_win = 0.0;
    double largest_loss = 0.0;
    int winning_trades = 0;
    int losing_trades = 0;
    
    for (const auto& trade : trades_) {
        total_pnl += trade.pnl;
        
        if (trade.pnl > 0) {
            total_wins += trade.pnl;
            winning_trades++;
            if (trade.pnl > largest_win) {
                largest_win = trade.pnl;
            }
        } else {
            total_losses += std::abs(trade.pnl);
            losing_trades++;
            if (std::abs(trade.pnl) > largest_loss) {
                largest_loss = std::abs(trade.pnl);
            }
        }
    }
    
    metrics_.total_pnl = total_pnl;
    metrics_.winning_trades = winning_trades;
    metrics_.losing_trades = losing_trades;
    metrics_.largest_win = largest_win;
    metrics_.largest_loss = largest_loss;
    
    if (winning_trades > 0) {
        metrics_.avg_win = total_wins / winning_trades;
    }
    if (losing_trades > 0) {
        metrics_.avg_loss = total_losses / losing_trades;
    }
}

void PerformanceAnalytics::update_latency_statistics() {
    if (latency_measurements_.empty()) return;
    
    double total_latency = 0.0;
    double max_latency = 0.0;
    double min_latency = std::numeric_limits<double>::max();
    
    for (const auto& latency : latency_measurements_) {
        double latency_us = latency.latency.count();
        total_latency += latency_us;
        
        if (latency_us > max_latency) {
            max_latency = latency_us;
        }
        if (latency_us < min_latency) {
            min_latency = latency_us;
        }
    }
    
    metrics_.avg_latency = total_latency / latency_measurements_.size();
    metrics_.max_latency = max_latency;
    metrics_.min_latency = min_latency;
}

void PerformanceAnalytics::log_trade(const Trade& trade) {
    trade_log_ << trade.trade_id << ","
               << trade.symbol << ","
               << (trade.side == OrderSide::BUY ? "BUY" : "SELL") << ","
               << trade.entry_price << ","
               << trade.exit_price << ","
               << trade.quantity << ","
               << trade.pnl << ","
               << trade.entry_time.time_since_epoch().count() << ","
               << trade.exit_time.time_since_epoch().count() << ","
               << trade.latency.count() << "\n";
    trade_log_.flush();
}

void PerformanceAnalytics::log_performance() {
    if (!performance_log_.is_open()) return;
    
    performance_log_ << "Timestamp,TotalPnL,SharpeRatio,MaxDrawdown,WinRate,TotalTrades\n";
    performance_log_ << std::chrono::high_resolution_clock::now().time_since_epoch().count() << ","
                     << metrics_.total_pnl << ","
                     << metrics_.sharpe_ratio << ","
                     << metrics_.max_drawdown << ","
                     << metrics_.win_rate << ","
                     << metrics_.total_trades << "\n";
    performance_log_.flush();
}

// PerformanceMonitor implementation
PerformanceMonitor::PerformanceMonitor(std::shared_ptr<PerformanceAnalytics> analytics,
                                     std::shared_ptr<OrderManager> order_manager)
    : analytics_(analytics), order_manager_(order_manager), running_(false),
      current_equity_(0.0), current_pnl_(0.0) {
    update_interval_ = std::chrono::milliseconds(1000);
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

void PerformanceMonitor::start() {
    if (!running_) {
        running_ = true;
        monitor_thread_ = std::thread(&PerformanceMonitor::monitor_loop, this);
    }
}

void PerformanceMonitor::stop() {
    if (running_) {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

void PerformanceMonitor::add_strategy(std::shared_ptr<TradingStrategy> strategy) {
    strategies_.push_back(strategy);
}

void PerformanceMonitor::update_price(const std::string& symbol, double price) {
    current_prices_[symbol] = price;
    analytics_->update_position(symbol, 0, price);
}

void PerformanceMonitor::update_position(const std::string& symbol, const Position& position) {
    current_positions_[symbol] = position;
}

void PerformanceMonitor::monitor_loop() {
    while (running_) {
        update_equity();
        update_unrealized_pnl();
        
        std::this_thread::sleep_for(update_interval_);
    }
}

void PerformanceMonitor::update_equity() {
    current_equity_ = order_manager_->get_total_pnl();
    analytics_->update_equity_curve(current_equity_);
}

void PerformanceMonitor::update_unrealized_pnl() {
    current_pnl_ = 0.0;
    for (const auto& [symbol, position] : current_positions_) {
        auto price_it = current_prices_.find(symbol);
        if (price_it != current_prices_.end()) {
            double current_price = price_it->second;
            double unrealized_pnl = (current_price - position.avg_price) * position.quantity;
            current_pnl_ += unrealized_pnl;
        }
    }
}

// DashboardDataProvider implementation
DashboardDataProvider::DashboardDataProvider(std::shared_ptr<PerformanceAnalytics> analytics,
                                           std::shared_ptr<OrderManager> order_manager,
                                           std::shared_ptr<PerformanceMonitor> monitor)
    : analytics_(analytics), order_manager_(order_manager), monitor_(monitor) {
}

DashboardData DashboardDataProvider::get_dashboard_data() const {
    DashboardData data;
    
    data.performance = analytics_->get_performance_metrics();
    data.risk = analytics_->get_risk_metrics();
    data.positions = monitor_->get_current_positions();
    data.prices = std::map<std::string, double>{}; // Replace with monitor_->get_current_prices() if available
    data.equity_curve = get_equity_curve();
    data.drawdown_curve = get_drawdown_curve();
    data.current_equity = monitor_->get_current_equity();
    data.current_pnl = monitor_->get_current_pnl();
    
    return data;
}

std::vector<double> DashboardDataProvider::get_equity_curve(uint32_t points) const {
    // Simplified implementation
    std::vector<double> curve;
    for (uint32_t i = 0; i < points; ++i) {
        curve.push_back(1000.0 + i * 10.0); // Mock data
    }
    return curve;
}

std::vector<double> DashboardDataProvider::get_drawdown_curve(uint32_t points) const {
    // Simplified implementation
    std::vector<double> curve;
    for (uint32_t i = 0; i < points; ++i) {
        curve.push_back(0.0); // Mock data
    }
    return curve;
}

std::vector<Trade> DashboardDataProvider::get_recent_trades(uint32_t count) const {
    auto trades = analytics_->get_trades();
    if (trades.size() <= count) {
        return trades;
    }
    return std::vector<Trade>(trades.end() - count, trades.end());
}

std::vector<LatencyMeasurement> DashboardDataProvider::get_recent_latencies(uint32_t count) const {
    auto latencies = analytics_->get_latency_measurements();
    if (latencies.size() <= count) {
        return latencies;
    }
    return std::vector<LatencyMeasurement>(latencies.end() - count, latencies.end());
}

// Enhanced analysis implementations
PnLHistogram PerformanceAnalytics::get_pnl_histogram(int num_bins) const {
    PnLHistogram histogram;
    if (pnl_history_.empty()) return histogram;
    
    auto min_max = std::minmax_element(pnl_history_.begin(), pnl_history_.end());
    histogram.min_pnl = *min_max.first;
    histogram.max_pnl = *min_max.second;
    histogram.bin_width = (histogram.max_pnl - histogram.min_pnl) / num_bins;
    
    histogram.bins.resize(num_bins);
    histogram.frequencies.resize(num_bins, 0);
    
    for (int i = 0; i < num_bins; ++i) {
        histogram.bins[i] = histogram.min_pnl + i * histogram.bin_width;
    }
    
    for (double pnl : pnl_history_) {
        int bin_index = static_cast<int>((pnl - histogram.min_pnl) / histogram.bin_width);
        if (bin_index >= 0 && bin_index < num_bins) {
            histogram.frequencies[bin_index]++;
        }
    }
    
    return histogram;
}

RiskMetrics PerformanceAnalytics::calculate_risk_metrics() const {
    RiskMetrics metrics;
    if (pnl_history_.size() < 2) return metrics;
    
    // Calculate returns
    std::vector<double> returns;
    for (size_t i = 1; i < pnl_history_.size(); ++i) {
        returns.push_back(pnl_history_[i] - pnl_history_[i-1]);
    }
    
    // Calculate VaR
    metrics.var_95 = calculate_var(returns, 0.95);
    metrics.var_99 = calculate_var(returns, 0.99);
    
    // Calculate Sharpe ratio
    metrics.sharpe_ratio = calculate_sharpe_ratio(returns);
    
    // Calculate max drawdown
    metrics.max_drawdown = calculate_max_drawdown(pnl_history_);
    
    // Calculate volatility
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    metrics.volatility = sqrt(variance / returns.size());
    
    // Calculate skewness and kurtosis
    metrics.skewness = calculate_skewness(returns);
    metrics.kurtosis = calculate_kurtosis(returns);
    
    // Calculate current exposure
    metrics.exposure = pnl_history_.empty() ? 0.0 : pnl_history_.back();
    
    return metrics;
}

std::vector<TradeLog> PerformanceAnalytics::get_trade_logs() const {
    return trade_logs_;
}

std::vector<OrderBookSnapshot> PerformanceAnalytics::get_order_book_snapshots() const {
    return book_snapshots_;
}

void PerformanceAnalytics::capture_order_book_snapshot(const std::string& symbol, const OrderBook& book) {
    if (!detailed_logging_enabled_) return;
    
    OrderBookSnapshot snapshot;
    snapshot.symbol = symbol;
    snapshot.timestamp = std::chrono::high_resolution_clock::now();
    snapshot.best_bid = book.get_best_bid();
    snapshot.best_ask = book.get_best_ask();
    snapshot.mid_price = book.get_mid_price();
    snapshot.spread = book.get_spread();
    snapshot.bid_levels = book.get_bid_levels(5);
    snapshot.ask_levels = book.get_ask_levels(5);
    
    book_snapshots_.push_back(snapshot);
}

void PerformanceAnalytics::enable_detailed_logging(bool enable) {
    detailed_logging_enabled_ = enable;
}

void PerformanceAnalytics::set_risk_limits(double max_drawdown, double max_exposure) {
    max_drawdown_limit_ = max_drawdown;
    max_exposure_limit_ = max_exposure;
}

void PerformanceAnalytics::export_risk_report(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    RiskMetrics metrics = calculate_risk_metrics();
    PnLHistogram histogram = get_pnl_histogram();
    
    file << "Risk Report\n";
    file << "===========\n\n";
    file << "VaR (95%): " << std::fixed << std::setprecision(2) << metrics.var_95 << "\n";
    file << "VaR (99%): " << metrics.var_99 << "\n";
    file << "Sharpe Ratio: " << metrics.sharpe_ratio << "\n";
    file << "Max Drawdown: " << metrics.max_drawdown << "\n";
    file << "Volatility: " << metrics.volatility << "\n";
    file << "Skewness: " << metrics.skewness << "\n";
    file << "Kurtosis: " << metrics.kurtosis << "\n";
    file << "Current Exposure: " << metrics.exposure << "\n\n";
    
    file << "PnL Distribution\n";
    file << "================\n";
    for (size_t i = 0; i < histogram.bins.size(); ++i) {
        file << "[" << histogram.bins[i] << ", " << (histogram.bins[i] + histogram.bin_width) 
             << "): " << histogram.frequencies[i] << "\n";
    }
}

void PerformanceAnalytics::export_trade_analysis(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "Trade Analysis\n";
    file << "==============\n\n";
    file << "Trade ID,Symbol,Side,Price,Quantity,PnL,Timestamp,Trader,Strategy\n";
    
    for (const auto& log : trade_logs_) {
        file << log.trade_id << "," << log.symbol << "," 
             << (log.side == OrderSide::BUY ? "BUY" : "SELL") << ","
             << std::fixed << std::setprecision(2) << log.price << ","
             << log.quantity << "," << log.pnl << ","
             << log.timestamp.time_since_epoch().count() << ","
             << log.trader_id << "," << log.strategy << "\n";
    }
}

void PerformanceAnalytics::print_performance_summary() const {
    RiskMetrics metrics = calculate_risk_metrics();
    PnLHistogram histogram = get_pnl_histogram();
    
    std::cout << "\n=== PERFORMANCE SUMMARY ===\n";
    std::cout << "Total Trades: " << trade_logs_.size() << "\n";
    std::cout << "Total PnL: " << std::fixed << std::setprecision(2) 
              << (pnl_history_.empty() ? 0.0 : pnl_history_.back()) << "\n";
    std::cout << "VaR (95%): " << metrics.var_95 << "\n";
    std::cout << "Sharpe Ratio: " << metrics.sharpe_ratio << "\n";
    std::cout << "Max Drawdown: " << metrics.max_drawdown << "\n";
    std::cout << "Volatility: " << metrics.volatility << "\n";
    std::cout << "Order Book Snapshots: " << book_snapshots_.size() << "\n";
    std::cout << "==========================\n\n";
}

// Helper functions
double PerformanceAnalytics::calculate_var(const std::vector<double>& returns, double confidence) const {
    if (returns.empty()) return 0.0;
    
    std::vector<double> sorted_returns(returns.begin(), returns.end());
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    int index = static_cast<int>((1.0 - confidence) * sorted_returns.size());
    return sorted_returns[index];
}

double PerformanceAnalytics::calculate_sharpe_ratio(const std::vector<double>& returns) const {
    if (returns.empty()) return 0.0;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    double std_dev = sqrt(variance / returns.size());
    
    return std_dev > 0 ? mean / std_dev : 0.0;
}

double PerformanceAnalytics::calculate_skewness(const std::vector<double>& values) const {
    if (values.size() < 3) return 0.0;
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0, third_moment = 0.0;
    
    for (double val : values) {
        double diff = val - mean;
        variance += diff * diff;
        third_moment += diff * diff * diff;
    }
    
    variance /= values.size();
    third_moment /= values.size();
    
    double std_dev = sqrt(variance);
    return std_dev > 0 ? third_moment / (std_dev * std_dev * std_dev) : 0.0;
}

double PerformanceAnalytics::calculate_kurtosis(const std::vector<double>& values) const {
    if (values.size() < 4) return 0.0;
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0, fourth_moment = 0.0;
    
    for (double val : values) {
        double diff = val - mean;
        variance += diff * diff;
        fourth_moment += diff * diff * diff * diff;
    }
    
    variance /= values.size();
    fourth_moment /= values.size();
    
    double std_dev = sqrt(variance);
    return std_dev > 0 ? fourth_moment / (std_dev * std_dev * std_dev * std_dev) - 3.0 : 0.0;
}

double PerformanceAnalytics::calculate_max_drawdown(const std::vector<double>& equity_curve) const {
    if (equity_curve.empty()) return 0.0;
    
    double max_drawdown = 0.0;
    double peak = equity_curve[0];
    
    for (double value : equity_curve) {
        if (value > peak) {
            peak = value;
        }
        double drawdown = peak - value;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }
    
    return max_drawdown;
}

} // namespace velocity 