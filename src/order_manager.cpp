#include "../include/order_manager.h"
#include <algorithm>
#include <iostream>

namespace velocity {

// MatchingEngine implementation
MatchingEngine::MatchingEngine() 
    : running_(false), total_orders_processed_(0), total_executions_(0), total_volume_(0.0),
      order_id_counter_(0), execution_id_counter_(0) {
}

MatchingEngine::~MatchingEngine() {
    stop();
}

void MatchingEngine::start() {
    if (!running_) {
        running_ = true;
        matching_thread_ = std::thread(&MatchingEngine::matching_loop, this);
    }
}

void MatchingEngine::stop() {
    if (running_) {
        running_ = false;
        order_cv_.notify_all();
        if (matching_thread_.joinable()) {
            matching_thread_.join();
        }
    }
}

void MatchingEngine::add_symbol(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    order_books_.emplace(symbol, OrderBook(symbol));
}

uint64_t MatchingEngine::submit_order(const Order& order) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    Order new_order = order;
    new_order.id = generate_order_id();
    new_order.timestamp = std::chrono::high_resolution_clock::now();
    
    order_queue_.push(new_order);
    order_cv_.notify_one();
    
    total_orders_processed_++;
    return new_order.id;
}

bool MatchingEngine::cancel_order(uint64_t order_id, const std::string& trader_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Find the order in the queue and remove it
    std::queue<Order> temp_queue;
    bool found = false;
    
    while (!order_queue_.empty()) {
        Order order = order_queue_.front();
        order_queue_.pop();
        
        if (order.id == order_id && order.trader_id == trader_id) {
            found = true;
            // Don't push this order back
        } else {
            temp_queue.push(order);
        }
    }
    
    // Restore the queue
    order_queue_ = temp_queue;
    
    return found;
}

bool MatchingEngine::modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, const std::string& trader_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Find and modify the order in the queue
    std::queue<Order> temp_queue;
    bool found = false;
    
    while (!order_queue_.empty()) {
        Order order = order_queue_.front();
        order_queue_.pop();
        
        if (order.id == order_id && order.trader_id == trader_id) {
            order.price = new_price;
            order.quantity = new_quantity;
            found = true;
        }
        
        temp_queue.push(order);
    }
    
    // Restore the queue
    order_queue_ = temp_queue;
    
    return found;
}

OrderBook& MatchingEngine::get_order_book(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return order_books_[symbol];
}

const OrderBook& MatchingEngine::get_order_book(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return order_books_.at(symbol);
}

void MatchingEngine::set_execution_callback(std::function<void(const Execution&)> callback) {
    execution_callback_ = callback;
}

void MatchingEngine::set_order_status_callback(std::function<void(const Order&)> callback) {
    order_status_callback_ = callback;
}

void MatchingEngine::matching_loop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(engine_mutex_);
        order_cv_.wait(lock, [this] { return !order_queue_.empty() || !running_; });
        
        if (!running_) break;
        
        while (!order_queue_.empty()) {
            Order order = order_queue_.front();
            order_queue_.pop();
            lock.unlock();
            
            process_order(order);
            
            lock.lock();
        }
    }
}

void MatchingEngine::process_order(const Order& order) {
    if (order.type == OrderType::MARKET) {
        // Market orders should immediately match against the best available prices
        process_market_order(order);
    } else {
        // Add limit orders to the appropriate order book
        auto& order_book = get_order_book(order.symbol);
        order_book.add_order(order);
        
        // Try to match orders
        match_orders(order.symbol);
    }
    
    // Notify order status
    if (order_status_callback_) {
        order_status_callback_(order);
    }
}

void MatchingEngine::process_market_order(const Order& order) {
    auto& order_book = get_order_book(order.symbol);
    
    if (order.side == OrderSide::BUY) {
        // Market buy order - match against best ask
        double best_ask = order_book.get_best_ask();
        if (best_ask > 0) {
            // Get the ask level to access actual orders
            auto ask_levels = order_book.get_ask_levels(1);
            if (!ask_levels.empty()) {
                auto& best_ask_level = ask_levels[0];
                uint32_t match_quantity = std::min(order.quantity, best_ask_level.total_quantity);
                
                if (match_quantity > 0) {
                    // Create execution
                    Execution execution;
                    execution.execution_id = generate_execution_id();
                    execution.symbol = order.symbol;
                    execution.side = order.side;
                    execution.price = best_ask;
                    execution.quantity = match_quantity;
                    execution.timestamp = std::chrono::high_resolution_clock::now();
                    execution.trader_id = order.trader_id;
                    execution.order_id = order.id;
                    
                    total_executions_++;
                    total_volume_ += execution.price * execution.quantity;
                    
                    // Remove matched orders from order book
                    if (!best_ask_level.orders.empty()) {
                        order_book.cancel_order(best_ask_level.orders[0].id);
                    }
                    
                    if (execution_callback_) {
                        execution_callback_(execution);
                    }
                }
            }
        }
    } else if (order.side == OrderSide::SELL) {
        // Market sell order - match against best bid
        double best_bid = order_book.get_best_bid();
        if (best_bid > 0) {
            // Get the bid level to access actual orders
            auto bid_levels = order_book.get_bid_levels(1);
            if (!bid_levels.empty()) {
                auto& best_bid_level = bid_levels[0];
                uint32_t match_quantity = std::min(order.quantity, best_bid_level.total_quantity);
                
                if (match_quantity > 0) {
                    // Create execution
                    Execution execution;
                    execution.execution_id = generate_execution_id();
                    execution.symbol = order.symbol;
                    execution.side = order.side;
                    execution.price = best_bid;
                    execution.quantity = match_quantity;
                    execution.timestamp = std::chrono::high_resolution_clock::now();
                    execution.trader_id = order.trader_id;
                    execution.order_id = order.id;
                    
                    total_executions_++;
                    total_volume_ += execution.price * execution.quantity;
                    
                    // Remove matched orders from order book
                    if (!best_bid_level.orders.empty()) {
                        order_book.cancel_order(best_bid_level.orders[0].id);
                    }
                    
                    if (execution_callback_) {
                        execution_callback_(execution);
                    }
                }
            }
        }
    }
}

void MatchingEngine::match_orders(const std::string& symbol) {
    auto& order_book = get_order_book(symbol);
    
    double best_bid = order_book.get_best_bid();
    double best_ask = order_book.get_best_ask();
    
    // Check if we have a valid order book
    if (best_bid <= 0 || best_ask <= 0) {
        return;
    }
    
    // Check if orders can be matched
    if (best_bid >= best_ask) {
        // Get the order book levels to access actual orders
        auto bid_levels = order_book.get_bid_levels(1);
        auto ask_levels = order_book.get_ask_levels(1);
        
        if (!bid_levels.empty() && !ask_levels.empty()) {
            auto& best_bid_level = bid_levels[0];
            auto& best_ask_level = ask_levels[0];
            
            // Match orders at the best prices
            uint32_t match_quantity = std::min(best_bid_level.total_quantity, best_ask_level.total_quantity);
            double match_price = (best_bid + best_ask) / 2.0;
            
            if (match_quantity > 0) {
                // Create execution
                Execution execution;
                execution.execution_id = generate_execution_id();
                execution.symbol = symbol;
                execution.price = match_price;
                execution.quantity = match_quantity;
                execution.timestamp = std::chrono::high_resolution_clock::now();
                
                total_executions_++;
                total_volume_ += execution.price * execution.quantity;
                
                // Remove matched orders from order book
                // For simplicity, we'll remove the entire price levels
                // In a real system, you'd match specific orders
                order_book.cancel_order(best_bid_level.orders[0].id);
                order_book.cancel_order(best_ask_level.orders[0].id);
                
                if (execution_callback_) {
                    execution_callback_(execution);
                }
            }
        }
    }
}

void MatchingEngine::execute_trade(const Order& buy_order, const Order& sell_order, double price, uint32_t quantity) {
    Execution execution;
    execution.order_id = buy_order.id;
    execution.execution_id = generate_execution_id();
    execution.symbol = buy_order.symbol;
    execution.side = buy_order.side;
    execution.price = price;
    execution.quantity = quantity;
    execution.timestamp = std::chrono::high_resolution_clock::now();
    execution.trader_id = buy_order.trader_id;
    
    total_executions_++;
    total_volume_ += price * quantity;
    
    if (execution_callback_) {
        execution_callback_(execution);
    }
}

uint64_t MatchingEngine::generate_order_id() {
    return ++order_id_counter_;
}

uint64_t MatchingEngine::generate_execution_id() {
    return ++execution_id_counter_;
}

// OrderManager implementation
OrderManager::OrderManager() : daily_pnl_(0.0), max_drawdown_(0.0), peak_equity_(0.0) {
    // Set up callbacks
    matching_engine_.set_execution_callback([this](const Execution& exec) { on_execution(exec); });
    matching_engine_.set_order_status_callback([this](const Order& order) { on_order_status(order); });
}

uint64_t OrderManager::place_order(const Order& order) {
    if (!check_risk_limits(order)) {
        return 0;
    }
    
    uint64_t order_id = matching_engine_.submit_order(order);
    
    if (order_id > 0) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        active_orders_[order.trader_id][order_id] = order;
    }
    
    return order_id;
}

bool OrderManager::cancel_order(uint64_t order_id, const std::string& trader_id) {
    bool cancelled = matching_engine_.cancel_order(order_id, trader_id);
    
    if (cancelled) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto& trader_orders = active_orders_[trader_id];
        trader_orders.erase(order_id);
    }
    
    return cancelled;
}

bool OrderManager::modify_order(uint64_t order_id, double new_price, uint32_t new_quantity, const std::string& trader_id) {
    bool modified = matching_engine_.modify_order(order_id, new_price, new_quantity, trader_id);
    
    if (modified) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto& trader_orders = active_orders_[trader_id];
        auto it = trader_orders.find(order_id);
        if (it != trader_orders.end()) {
            it->second.price = new_price;
            it->second.quantity = new_quantity;
        }
    }
    
    return modified;
}

Position OrderManager::get_position(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }
    return Position();
}

std::vector<Position> OrderManager::get_all_positions() const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    std::vector<Position> result;
    for (const auto& [symbol, position] : positions_) {
        result.push_back(position);
    }
    return result;
}

void OrderManager::update_position(const Execution& execution) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto& position = positions_[execution.symbol];
    position.symbol = execution.symbol;
    
    if (execution.side == OrderSide::BUY) {
        position.quantity += execution.quantity;
    } else {
        position.quantity -= execution.quantity;
    }
    
    // Update average price (simplified)
    if (position.quantity != 0) {
        position.avg_price = execution.price;
    }
    
    if (position_callback_) {
        position_callback_(position);
    }
}

bool OrderManager::check_risk_limits(const Order& order) {
    return check_position_limits(order) && check_daily_loss_limit(order);
}

void OrderManager::set_risk_limits(const RiskLimits& limits) {
    risk_limits_ = limits;
}

double OrderManager::get_total_pnl() const {
    double total = 0.0;
    for (const auto& [symbol, position] : positions_) {
        total += position.realized_pnl + position.unrealized_pnl;
    }
    return total;
}

void OrderManager::set_execution_callback(std::function<void(const Execution&)> callback) {
    execution_callback_ = callback;
}

void OrderManager::set_position_callback(std::function<void(const Position&)> callback) {
    position_callback_ = callback;
}

void OrderManager::set_risk_alert_callback(std::function<void(const std::string&)> callback) {
    risk_alert_callback_ = callback;
}

OrderBook& OrderManager::get_order_book(const std::string& symbol) {
    return matching_engine_.get_order_book(symbol);
}

const OrderBook& OrderManager::get_order_book(const std::string& symbol) const {
    return matching_engine_.get_order_book(symbol);
}

void OrderManager::on_execution(const Execution& execution) {
    update_position(execution);
    
    if (execution_callback_) {
        execution_callback_(execution);
    }
}

void OrderManager::on_order_status(const Order& order) {
    // Update order status in active orders
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto& trader_orders = active_orders_[order.trader_id];
    auto it = trader_orders.find(order.id);
    if (it != trader_orders.end()) {
        it->second = order;
    }
}

bool OrderManager::check_position_limits(const Order& order) {
    // Simplified position limit check
    auto current_position = get_position(order.symbol);
    int32_t new_quantity = current_position.quantity;
    
    if (order.side == OrderSide::BUY) {
        new_quantity += order.quantity;
    } else {
        new_quantity -= order.quantity;
    }
    
    return std::abs(new_quantity) <= risk_limits_.max_order_size;
}

bool OrderManager::check_daily_loss_limit(const Order& order) {
    return daily_pnl_ > -risk_limits_.max_daily_loss;
}

void OrderManager::update_drawdown() {
    double current_equity = get_total_pnl();
    if (current_equity > peak_equity_) {
        peak_equity_ = current_equity;
    }
    
    if (peak_equity_ > 0) {
        double drawdown = (peak_equity_ - current_equity) / peak_equity_;
        if (drawdown > max_drawdown_) {
            max_drawdown_ = drawdown;
        }
    }
}

void OrderManager::send_risk_alert(const std::string& message) {
    if (risk_alert_callback_) {
        risk_alert_callback_(message);
    }
}

void OrderManager::add_symbol(const std::string& symbol) {
    matching_engine_.add_symbol(symbol);
}

} // namespace velocity 