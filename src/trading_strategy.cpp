#include "../include/trading_strategy.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace velocity {

// TradingStrategy base class implementation
TradingStrategy::TradingStrategy(const std::string& name, const std::string& trader_id, 
                               std::shared_ptr<OrderManager> order_manager)
    : name_(name), trader_id_(trader_id), order_manager_(order_manager), running_(false) {
    start_time_ = std::chrono::high_resolution_clock::now();
}

void TradingStrategy::add_symbol(const std::string& symbol) {
    symbols_.push_back(symbol);
}

void TradingStrategy::set_trader_id(const std::string& trader_id) {
    trader_id_ = trader_id;
}

StrategyMetrics TradingStrategy::get_metrics() const {
    return metrics_;
}

void TradingStrategy::update_metrics(const Execution& execution) {
    metrics_.total_trades++;
    
    // Calculate P&L (simplified)
    double pnl = 0.0;
    if (execution.side == OrderSide::BUY) {
        pnl = -execution.price * execution.quantity;
    } else {
        pnl = execution.price * execution.quantity;
    }
    
    metrics_.total_pnl += pnl;
    
    if (pnl > 0) {
        metrics_.winning_trades++;
    }
    
    // Update win rate
    if (metrics_.total_trades > 0) {
        metrics_.win_rate = static_cast<double>(metrics_.winning_trades) / metrics_.total_trades;
    }
}

uint64_t TradingStrategy::place_market_order(const std::string& symbol, OrderSide side, uint32_t quantity) {
    Order order;
    order.symbol = symbol;
    order.side = side;
    order.type = OrderType::MARKET;
    order.quantity = quantity;
    order.trader_id = trader_id_;
    
    return order_manager_->place_order(order);
}

uint64_t TradingStrategy::place_limit_order(const std::string& symbol, OrderSide side, double price, uint32_t quantity) {
    Order order;
    order.symbol = symbol;
    order.side = side;
    order.type = OrderType::LIMIT;
    order.price = price;
    order.quantity = quantity;
    order.trader_id = trader_id_;
    
    return order_manager_->place_order(order);
}

bool TradingStrategy::cancel_order(uint64_t order_id) {
    return order_manager_->cancel_order(order_id, trader_id_);
}

bool TradingStrategy::check_position_limit(const std::string& symbol, int32_t additional_quantity) {
    auto position = order_manager_->get_position(symbol);
    return std::abs(position.quantity + additional_quantity) <= 10000; // Simplified limit
}

double TradingStrategy::get_position_value(const std::string& symbol) {
    auto position = order_manager_->get_position(symbol);
    return std::abs(position.quantity * position.avg_price);
}

double TradingStrategy::get_best_bid(const std::string& symbol) {
    return order_manager_->get_order_book(symbol).get_best_bid();
}

double TradingStrategy::get_best_ask(const std::string& symbol) {
    return order_manager_->get_order_book(symbol).get_best_ask();
}

double TradingStrategy::get_mid_price(const std::string& symbol) {
    return order_manager_->get_order_book(symbol).get_mid_price();
}

double TradingStrategy::get_spread(const std::string& symbol) {
    return order_manager_->get_order_book(symbol).get_spread();
}

void TradingStrategy::calculate_sharpe_ratio() {
    // Simplified Sharpe ratio calculation
    if (returns_history_.size() > 1) {
        double mean_return = 0.0;
        for (double ret : returns_history_) {
            mean_return += ret;
        }
        mean_return /= returns_history_.size();
        
        double variance = 0.0;
        for (double ret : returns_history_) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        variance /= (returns_history_.size() - 1);
        
        if (variance > 0) {
            metrics_.sharpe_ratio = mean_return / std::sqrt(variance);
        }
    }
}

void TradingStrategy::calculate_win_rate() {
    if (metrics_.total_trades > 0) {
        metrics_.win_rate = static_cast<double>(metrics_.winning_trades) / metrics_.total_trades;
    }
}

void TradingStrategy::update_drawdown(double current_pnl) {
    static double peak_pnl = 0.0;
    
    if (current_pnl > peak_pnl) {
        peak_pnl = current_pnl;
    }
    
    if (peak_pnl > 0) {
        double drawdown = (peak_pnl - current_pnl) / peak_pnl;
        if (drawdown > metrics_.max_drawdown) {
            metrics_.max_drawdown = drawdown;
        }
    }
}

// MarketMakingStrategy implementation
MarketMakingStrategy::MarketMakingStrategy(const std::string& name, const std::string& trader_id,
                                         std::shared_ptr<OrderManager> order_manager)
    : TradingStrategy(name, trader_id, order_manager),
      spread_multiplier_(1.5), base_quantity_(1000), max_position_(10000), min_spread_(0.01) {
}

void MarketMakingStrategy::initialize() {
    // Initialize market making strategy
}

void MarketMakingStrategy::start() {
    running_ = true;
}

void MarketMakingStrategy::stop() {
    running_ = false;
    
    // Cancel all active orders
    for (const auto& [symbol, order_id] : active_bid_orders_) {
        cancel_order(order_id);
    }
    for (const auto& [symbol, order_id] : active_ask_orders_) {
        cancel_order(order_id);
    }
    
    active_bid_orders_.clear();
    active_ask_orders_.clear();
}

void MarketMakingStrategy::on_market_data(const std::string& symbol, double bid, double ask) {
    if (!running_) return;
    
    update_quotes(symbol);
}

void MarketMakingStrategy::on_execution(const Execution& execution) {
    update_metrics(execution);
    
    // Update active orders
    if (execution.side == OrderSide::BUY) {
        active_bid_orders_.erase(execution.symbol);
    } else {
        active_ask_orders_.erase(execution.symbol);
    }
}

void MarketMakingStrategy::update_quotes(const std::string& symbol) {
    double mid_price = get_mid_price(symbol);
    if (mid_price <= 0) return;
    
    double spread = std::abs(get_spread(symbol));
    if (spread < min_spread_) {
        spread = min_spread_; // Use minimum spread
    }
    
    double bid_price = calculate_bid_price(symbol);
    double ask_price = calculate_ask_price(symbol);
    
    // Ensure bid < ask
    if (bid_price >= ask_price) {
        bid_price = mid_price - (spread / 2.0);
        ask_price = mid_price + (spread / 2.0);
    }
    
    // Cancel old quotes
    cancel_old_quotes(symbol);
    
    // Place new quotes
    uint32_t bid_quantity = calculate_quantity(symbol, OrderSide::BUY);
    uint32_t ask_quantity = calculate_quantity(symbol, OrderSide::SELL);
    
    if (bid_quantity > 0 && bid_price > 0) {
        uint64_t bid_id = place_limit_order(symbol, OrderSide::BUY, bid_price, bid_quantity);
        if (bid_id > 0) {
            active_bid_orders_[symbol] = bid_id;
            last_bid_prices_[symbol] = bid_price;
        }
    }
    
    if (ask_quantity > 0 && ask_price > 0) {
        uint64_t ask_id = place_limit_order(symbol, OrderSide::SELL, ask_price, ask_quantity);
        if (ask_id > 0) {
            active_ask_orders_[symbol] = ask_id;
            last_ask_prices_[symbol] = ask_price;
        }
    }
}

void MarketMakingStrategy::cancel_old_quotes(const std::string& symbol) {
    auto bid_it = active_bid_orders_.find(symbol);
    if (bid_it != active_bid_orders_.end()) {
        cancel_order(bid_it->second);
        active_bid_orders_.erase(bid_it);
    }
    
    auto ask_it = active_ask_orders_.find(symbol);
    if (ask_it != active_ask_orders_.end()) {
        cancel_order(ask_it->second);
        active_ask_orders_.erase(ask_it);
    }
}

double MarketMakingStrategy::calculate_bid_price(const std::string& symbol) {
    double mid_price = get_mid_price(symbol);
    double spread = std::abs(get_spread(symbol)); // Use absolute spread
    if (spread < min_spread_) {
        spread = min_spread_; // Use minimum spread if current spread is too small
    }
    return mid_price - (spread * spread_multiplier_ / 2.0);
}

double MarketMakingStrategy::calculate_ask_price(const std::string& symbol) {
    double mid_price = get_mid_price(symbol);
    double spread = std::abs(get_spread(symbol)); // Use absolute spread
    if (spread < min_spread_) {
        spread = min_spread_; // Use minimum spread if current spread is too small
    }
    return mid_price + (spread * spread_multiplier_ / 2.0);
}

uint32_t MarketMakingStrategy::calculate_quantity(const std::string& symbol, OrderSide side) {
    auto position = order_manager_->get_position(symbol);
    int32_t current_position = position.quantity;
    
    if (side == OrderSide::BUY && current_position >= max_position_) {
        return 0; // Already at max long position
    }
    
    if (side == OrderSide::SELL && current_position <= -max_position_) {
        return 0; // Already at max short position
    }
    
    return base_quantity_;
}

// StrategyFactory implementation
std::unique_ptr<TradingStrategy> StrategyFactory::create_strategy(
    const std::string& strategy_type,
    const std::string& name,
    const std::string& trader_id,
    std::shared_ptr<OrderManager> order_manager) {
    
    if (strategy_type == "market_making") {
        return std::make_unique<MarketMakingStrategy>(name, trader_id, order_manager);
    }
    else if (strategy_type == "momentum") {
        return std::make_unique<MomentumStrategy>(name, trader_id, order_manager);
    }
    else if (strategy_type == "market_orders") {
        return std::make_unique<MarketOrderStrategy>(name, trader_id, order_manager);
    }
    // Add other strategy types here as they are implemented
    
    return nullptr;
}

// StatArbStrategy implementation
StatArbStrategy::StatArbStrategy(const std::string& name, const std::string& trader_id, std::shared_ptr<OrderManager> order_manager)
    : TradingStrategy(name, trader_id, order_manager), z_score_threshold_(2.0), lookback_period_(20), position_size_(100) {}

void StatArbStrategy::initialize() {}
void StatArbStrategy::start() { running_ = true; }
void StatArbStrategy::stop() { running_ = false; }
void StatArbStrategy::on_market_data(const std::string& symbol, double bid, double ask) {}
void StatArbStrategy::on_execution(const Execution& execution) {}
void StatArbStrategy::set_pair_symbols(const std::string& symbol1, const std::string& symbol2) { pair_symbol1_ = symbol1; pair_symbol2_ = symbol2; }

// MomentumStrategy implementation
MomentumStrategy::MomentumStrategy(const std::string& name, const std::string& trader_id, std::shared_ptr<OrderManager> order_manager)
    : TradingStrategy(name, trader_id, order_manager), short_window_(5), long_window_(20), momentum_threshold_(0.02), position_size_(100) {}

void MomentumStrategy::initialize() {}
void MomentumStrategy::start() { running_ = true; }
void MomentumStrategy::stop() { running_ = false; }
void MomentumStrategy::on_market_data(const std::string& symbol, double bid, double ask) {}
void MomentumStrategy::on_execution(const Execution& execution) {}
void MomentumStrategy::set_windows(uint32_t short_window, uint32_t long_window) { short_window_ = short_window; long_window_ = long_window; }

// MarketOrderStrategy implementation
MarketOrderStrategy::MarketOrderStrategy(const std::string& name, const std::string& trader_id, std::shared_ptr<OrderManager> order_manager)
    : TradingStrategy(name, trader_id, order_manager), order_interval_ms_(2000), order_size_(500), max_orders_(10), order_count_(0) {}

void MarketOrderStrategy::initialize() {}
void MarketOrderStrategy::start() { 
    running_ = true; 
    last_order_time_ = std::chrono::high_resolution_clock::now();
}
void MarketOrderStrategy::stop() { running_ = false; }
void MarketOrderStrategy::on_market_data(const std::string& symbol, double bid, double ask) {
    if (!running_) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_order_time_);
    
    if (elapsed.count() >= order_interval_ms_ && order_count_ < max_orders_) {
        // Alternate between BUY and SELL
        OrderSide side = (order_count_ % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        uint64_t order_id = place_market_order(symbol, side, order_size_);
        if (order_id > 0) {
            std::cout << "[MarketOrderStrategy] Placed " << (side == OrderSide::BUY ? "BUY" : "SELL")
                      << " market order for " << symbol << " (size: " << order_size_ << ")\n";
            order_count_++;
            last_order_time_ = now;
        }
    }
}
void MarketOrderStrategy::on_execution(const Execution& execution) {
    update_metrics(execution);
}

} // namespace velocity 