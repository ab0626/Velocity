#include "../include/velocity_engine.h"
#include <iostream>

namespace velocity {

VelocityEngine::VelocityEngine(const VelocityConfig& config) 
    : running_(false), dashboard_running_(false), config_(config) {
}

VelocityEngine::~VelocityEngine() {
    stop();
}

void VelocityEngine::initialize() {
    // Initialize components
    market_data_feed_ = std::make_shared<MarketDataFeed>();
    order_manager_ = std::make_shared<OrderManager>();
    analytics_ = std::make_shared<PerformanceAnalytics>();
    monitor_ = std::make_shared<PerformanceMonitor>(analytics_, order_manager_);
    dashboard_provider_ = std::make_shared<DashboardDataProvider>(analytics_, order_manager_, monitor_);
    
    // Add symbols
    for (const auto& symbol : config_.symbols) {
        auto it = config_.initial_prices.find(symbol);
        if (it != config_.initial_prices.end()) {
            market_data_feed_->add_symbol(symbol, it->second);
        }
    }
    
    // Start monitor
    monitor_->start();
    
    std::cout << "Velocity engine initialized\n";
}

void VelocityEngine::start() {
    if (!running_) {
        running_ = true;
        market_data_feed_->start();
        engine_thread_ = std::thread(&VelocityEngine::engine_loop, this);
        std::cout << "Velocity engine started\n";
    }
}

void VelocityEngine::stop() {
    if (running_) {
        running_ = false;
        market_data_feed_->stop();
        if (engine_thread_.joinable()) {
            engine_thread_.join();
        }
        std::cout << "Velocity engine stopped\n";
    }
}

void VelocityEngine::add_strategy(const std::string& strategy_type, const std::string& name, 
                                 const std::string& trader_id, 
                                 const std::map<std::string, std::string>& params) {
    auto strategy = StrategyFactory::create_strategy(strategy_type, name, trader_id, order_manager_);
    if (strategy) {
        strategy->initialize();
        strategy->start();
        strategy_map_[name] = std::move(strategy);
        std::cout << "Added strategy: " << name << "\n";
    }
}

void VelocityEngine::remove_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->stop();
        strategy_map_.erase(it);
    }
}

void VelocityEngine::start_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->start();
    }
}

void VelocityEngine::stop_strategy(const std::string& name) {
    auto it = strategy_map_.find(name);
    if (it != strategy_map_.end()) {
        it->second->stop();
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
    market_data_feed_->add_symbol(symbol, initial_price);
}

void VelocityEngine::remove_symbol(const std::string& symbol) {
    // Simplified implementation
}

std::vector<std::string> VelocityEngine::get_symbols() const {
    return config_.symbols;
}

uint64_t VelocityEngine::place_order(const Order& order) {
    return order_manager_->place_order(order);
}

bool VelocityEngine::cancel_order(uint64_t order_id, const std::string& trader_id) {
    return order_manager_->cancel_order(order_id, trader_id);
}

bool VelocityEngine::modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, 
                                 const std::string& trader_id) {
    return order_manager_->modify_order(order_id, new_price, new_quantity, trader_id);
}

PerformanceMetrics VelocityEngine::get_performance_metrics() const {
    return analytics_->get_performance_metrics();
}

RiskMetrics VelocityEngine::get_risk_metrics() const {
    return analytics_->get_risk_metrics();
}

std::vector<Position> VelocityEngine::get_positions() const {
    return order_manager_->get_all_positions();
}

DashboardData VelocityEngine::get_dashboard_data() const {
    return dashboard_provider_->get_dashboard_data();
}

OrderBook& VelocityEngine::get_order_book(const std::string& symbol) {
    return market_data_feed_->get_order_book(symbol);
}

const OrderBook& VelocityEngine::get_order_book(const std::string& symbol) const {
    return market_data_feed_->get_order_book(symbol);
}

void VelocityEngine::export_trades_to_csv(const std::string& filename) {
    analytics_->export_trades_to_csv(filename);
}

void VelocityEngine::export_performance_to_csv(const std::string& filename) {
    analytics_->export_performance_to_csv(filename);
}

void VelocityEngine::enable_logging(const std::string& directory) {
    analytics_->enable_trade_logging(directory + "/trades.csv");
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
    }
}

void VelocityEngine::engine_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void VelocityEngine::dashboard_server_loop(uint16_t port) {
    while (dashboard_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto data = get_dashboard_data();
        std::cout << "Dashboard - P&L: $" << data.current_pnl << "\n";
    }
}

std::string VelocityEngine::generate_dashboard_html() const {
    return "<html><body><h1>Velocity Dashboard</h1></body></html>";
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
    return "2024-01-01 00:00:00"; // Simplified
}

uint64_t get_current_time_microseconds() {
    return 0; // Simplified
}

double calculate_std_dev(const std::vector<double>& values) {
    return 0.0; // Simplified
}

double calculate_mean(const std::vector<double>& values) {
    return 0.0; // Simplified
}

double calculate_percentile(const std::vector<double>& values, double percentile) {
    return 0.0; // Simplified
}

std::string format_currency(double value) {
    return "$" + std::to_string(static_cast<int>(value)); // Simplified
}

std::string format_percentage(double value) {
    return std::to_string(static_cast<int>(value * 100)) + "%"; // Simplified
}

std::string format_number(double value, int precision) {
    return std::to_string(static_cast<int>(value)); // Simplified
}

bool create_directory(const std::string& path) {
    return true; // Simplified
}

std::string get_file_extension(const std::string& filename) {
    return ""; // Simplified
}

bool file_exists(const std::string& filename) {
    return true; // Simplified
}

} // namespace utils

} // namespace velocity 