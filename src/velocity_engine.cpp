#include "../include/velocity_engine.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace velocity {

// VelocityEngine implementation
VelocityEngine::VelocityEngine(const VelocityConfig& config) 
    : running_(false), dashboard_running_(false), config_(config) {
}

VelocityEngine::~VelocityEngine() {
    stop();
}

void VelocityEngine::initialize() {
    initialize_market_data();
    initialize_analytics();
    setup_callbacks();
}

void VelocityEngine::start() {
    if (!running_) {
        running_ = true;
        
        // Start market data feed
        market_data_feed_->start();
        
        // Start order manager
        order_manager_->get_order_book("AAPL"); // Initialize order books
        
        // Start engine thread
        engine_thread_ = std::thread(&VelocityEngine::engine_loop, this);
        
        std::cout << "Velocity engine started successfully\n";
    }
}

void VelocityEngine::stop() {
    if (running_) {
        running_ = false;
        
        // Stop all strategies
        for (auto& [name, strategy] : strategy_map_) {
            strategy->stop();
        }
        
        // Stop market data feed
        if (market_data_feed_) {
            market_data_feed_->stop();
        }
        
        // Stop dashboard
        stop_dashboard();
        
        // Join threads
        if (engine_thread_.joinable()) {
            engine_thread_.join();
        }
        
        std::cout << "Velocity engine stopped\n";
    }
}

void VelocityEngine::add_strategy(const std::string& strategy_type, const std::string& name, 
                                 const std::string& trader_id, 
                                 const std::map<std::string, std::string>& params) {
    auto strategy = create_strategy(strategy_type, name, trader_id, params);
    if (strategy) {
        strategy->initialize();
        strategy->start();
        
        strategy_map_[name] = std::move(strategy);
        std::cout << "Added strategy: " << name << " (" << strategy_type << ")\n";
    } else {
        std::cerr << "Failed to create strategy: " << strategy_type << "\n";
    }
}

void VelocityEngine::remove_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->stop();
        strategy_map_.erase(it);
        std::cout << "Removed strategy: " << name << "\n";
    }
}

void VelocityEngine::start_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->start();
        std::cout << "Started strategy: " << name << "\n";
    }
}

void VelocityEngine::stop_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->stop();
        std::cout << "Stopped strategy: " << name << "\n";
    }
}

std::vector<std::string> VelocityEngine::get_strategy_names() const {
    std::vector<std::string> names;
    for (const auto& [name, strategy] : strategy_map_) {
        names.push_back(name);
    }
    return names;
}

void VelocityEngine::add_symbol(const std::string& symbol, double initial_price) {
    if (market_data_feed_) {
        market_data_feed_->add_symbol(symbol, initial_price);
        std::cout << "Added symbol: " << symbol << " @ $" << initial_price << "\n";
    }
}

void VelocityEngine::remove_symbol(const std::string& symbol) {
    // Simplified implementation
    std::cout << "Removed symbol: " << symbol << "\n";
}

std::vector<std::string> VelocityEngine::get_symbols() const {
    // Simplified implementation - return configured symbols
    return config_.symbols;
}

uint64_t VelocityEngine::place_order(const Order& order) {
    if (order_manager_) {
        return order_manager_->place_order(order);
    }
    return 0;
}

bool VelocityEngine::cancel_order(uint64_t order_id, const std::string& trader_id) {
    if (order_manager_) {
        return order_manager_->cancel_order(order_id, trader_id);
    }
    return false;
}

bool VelocityEngine::modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, 
                                 const std::string& trader_id) {
    if (order_manager_) {
        return order_manager_->modify_order(order_id, new_price, new_quantity, trader_id);
    }
    return false;
}

PerformanceMetrics VelocityEngine::get_performance_metrics() const {
    if (analytics_) {
        return analytics_->get_performance_metrics();
    }
    return PerformanceMetrics();
}

RiskMetrics VelocityEngine::get_risk_metrics() const {
    if (analytics_) {
        return analytics_->get_risk_metrics();
    }
    return RiskMetrics();
}

std::vector<Position> VelocityEngine::get_positions() const {
    if (order_manager_) {
        return order_manager_->get_all_positions();
    }
    return std::vector<Position>();
}

DashboardData VelocityEngine::get_dashboard_data() const {
    if (dashboard_provider_) {
        return dashboard_provider_->get_dashboard_data();
    }
    return DashboardData();
}

OrderBook& VelocityEngine::get_order_book(const std::string& symbol) {
    if (market_data_feed_) {
        return market_data_feed_->get_order_book(symbol);
    }
    static OrderBook dummy_book("DUMMY");
    return dummy_book;
}

const OrderBook& VelocityEngine::get_order_book(const std::string& symbol) const {
    if (market_data_feed_) {
        return market_data_feed_->get_order_book(symbol);
    }
    static OrderBook dummy_book("DUMMY");
    return dummy_book;
}

void VelocityEngine::export_trades_to_csv(const std::string& filename) {
    if (analytics_) {
        analytics_->export_trades_to_csv(filename);
    }
}

void VelocityEngine::export_performance_to_csv(const std::string& filename) {
    if (analytics_) {
        analytics_->export_performance_to_csv(filename);
    }
}

void VelocityEngine::enable_logging(const std::string& directory) {
    if (analytics_) {
        analytics_->enable_trade_logging(directory + "/trades.csv");
        analytics_->enable_performance_logging(directory + "/performance.csv");
    }
}

void VelocityEngine::start_dashboard(uint16_t port) {
    if (!dashboard_running_) {
        dashboard_running_ = true;
        dashboard_thread_ = std::thread(&VelocityEngine::dashboard_server_loop, this, port);
        std::cout << "Dashboard started on port " << port << "\n";
    }
}

void VelocityEngine::stop_dashboard() {
    if (dashboard_running_) {
        dashboard_running_ = false;
        if (dashboard_thread_.joinable()) {
            dashboard_thread_.join();
        }
        std::cout << "Dashboard stopped\n";
    }
}

void VelocityEngine::engine_loop() {
    while (running_) {
        // Main engine loop - simplified
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void VelocityEngine::initialize_market_data() {
    market_data_feed_ = std::make_shared<MarketDataFeed>();
    
    // Add configured symbols
    for (const auto& symbol : config_.symbols) {
        auto it = config_.initial_prices.find(symbol);
        if (it != config_.initial_prices.end()) {
            market_data_feed_->add_symbol(symbol, it->second);
        } else {
            market_data_feed_->add_symbol(symbol, 100.0); // Default price
        }
    }
}

void VelocityEngine::initialize_analytics() {
    analytics_ = std::make_shared<PerformanceAnalytics>();
    order_manager_ = std::make_shared<OrderManager>();
    monitor_ = std::make_shared<PerformanceMonitor>(analytics_, order_manager_);
    dashboard_provider_ = std::make_shared<DashboardDataProvider>(analytics_, order_manager_, monitor_);
    
    // Start monitor
    monitor_->start();
    
    // Enable logging if configured
    if (config_.enable_logging) {
        enable_logging(config_.log_directory);
    }
}

void VelocityEngine::setup_callbacks() {
    if (market_data_feed_ && order_manager_) {
        // Set up market data callbacks
        market_data_feed_->set_price_callback([this](const std::string& symbol, double bid, double ask) {
            // Update strategies with market data
            for (auto& [name, strategy] : strategy_map_) {
                strategy->on_market_data(symbol, bid, ask);
            }
            
            // Update monitor
            if (monitor_) {
                monitor_->update_price(symbol, (bid + ask) / 2.0);
            }
        });
        
        // Set up execution callbacks
        order_manager_->set_execution_callback([this](const Execution& execution) {
            // Update analytics
            if (analytics_) {
                // Create trade record
                Trade trade;
                trade.trade_id = execution.execution_id;
                trade.symbol = execution.symbol;
                trade.side = execution.side;
                trade.entry_price = execution.price;
                trade.exit_price = execution.price;
                trade.quantity = execution.quantity;
                trade.pnl = 0.0; // Simplified
                trade.entry_time = execution.timestamp;
                trade.exit_time = execution.timestamp;
                trade.latency = std::chrono::microseconds(0);
                
                analytics_->record_trade(trade);
            }
            
            // Update strategies
            for (auto& [name, strategy] : strategy_map_) {
                strategy->on_execution(execution);
            }
        });
    }
}

std::shared_ptr<TradingStrategy> VelocityEngine::create_strategy(const std::string& strategy_type,
                                                               const std::string& name,
                                                               const std::string& trader_id,
                                                               const std::map<std::string, std::string>& params) {
    return StrategyFactory::create_strategy(strategy_type, name, trader_id, order_manager_);
}

void VelocityEngine::dashboard_server_loop(uint16_t port) {
    // Simplified dashboard server - just print status
    while (dashboard_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        auto data = get_dashboard_data();
        std::cout << "Dashboard Status - P&L: $" << data.current_pnl 
                  << ", Equity: $" << data.current_equity << "\n";
    }
}

std::string VelocityEngine::generate_dashboard_html() const {
    return "<html><body><h1>Velocity HFT Dashboard</h1></body></html>";
}

std::string VelocityEngine::generate_dashboard_json() const {
    return "{\"status\": \"running\"}";
}

void VelocityEngine::log_engine_event(const std::string& event) {
    std::cout << "[VELOCITY] " << event << "\n";
}

void VelocityEngine::handle_error(const std::string& error) {
    std::cerr << "[VELOCITY ERROR] " << error << "\n";
}

std::shared_ptr<OrderManager> VelocityEngine::get_order_manager() {
    return order_manager_;
}

// VelocityEngineBuilder implementation
VelocityEngineBuilder::VelocityEngineBuilder() {
}

VelocityEngineBuilder& VelocityEngineBuilder::add_symbol(const std::string& symbol, double initial_price) {
    config_.symbols.push_back(symbol);
    config_.initial_prices[symbol] = initial_price;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_volatility(double multiplier) {
    config_.volatility_multiplier = multiplier;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_market_data_frequency(uint32_t ms) {
    config_.market_data_frequency_ms = ms;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_max_order_size(uint32_t size) {
    config_.max_order_size = size;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_max_position_value(double value) {
    config_.max_position_value = value;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_max_daily_loss(double loss) {
    config_.max_daily_loss = loss;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_max_drawdown(double drawdown) {
    config_.max_drawdown = drawdown;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::add_strategy(const std::string& strategy_type, 
                                                         const std::map<std::string, std::string>& params) {
    config_.enabled_strategies.push_back(strategy_type);
    config_.strategy_params[strategy_type] = params;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_strategy_param(const std::string& strategy, 
                                                               const std::string& param, 
                                                               const std::string& value) {
    config_.strategy_params[strategy][param] = value;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::enable_logging(const std::string& directory) {
    config_.enable_logging = true;
    config_.log_directory = directory;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::set_performance_update_frequency(uint32_t ms) {
    config_.performance_update_frequency_ms = ms;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::enable_dashboard(uint16_t port) {
    config_.enable_dashboard = true;
    config_.dashboard_port = port;
    return *this;
}

VelocityEngineBuilder& VelocityEngineBuilder::disable_dashboard() {
    config_.enable_dashboard = false;
    return *this;
}

std::unique_ptr<VelocityEngine> VelocityEngineBuilder::build() {
    return std::make_unique<VelocityEngine>(config_);
}

// Utility functions
namespace utils {

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

uint64_t get_current_time_microseconds() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
}

double calculate_std_dev(const std::vector<double>& values) {
    if (values.size() < 2) return 0.0;
    
    double mean = calculate_mean(values);
    double sum_sq = 0.0;
    
    for (double val : values) {
        sum_sq += (val - mean) * (val - mean);
    }
    
    return std::sqrt(sum_sq / (values.size() - 1));
}

double calculate_mean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double calculate_percentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) return 0.0;
    
    std::vector<double> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    
    int index = static_cast<int>(percentile * (sorted_values.size() - 1));
    if (index >= sorted_values.size()) index = sorted_values.size() - 1;
    
    return sorted_values[index];
}

std::string format_currency(double value) {
    std::stringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

std::string format_percentage(double value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (value * 100) << "%";
    return ss.str();
}

std::string format_number(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

bool create_directory(const std::string& path) {
    // Simplified implementation
    return true;
}

std::string get_file_extension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos + 1);
    }
    return "";
}

bool file_exists(const std::string& filename) {
    // Simplified implementation
    return true;
}

} // namespace utils

} // namespace velocity 