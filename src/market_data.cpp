#include "../include/market_data.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>

namespace velocity {

// OrderBook implementation
OrderBook::OrderBook() : symbol_(""), last_price_(0.0), best_bid_(0.0), best_ask_(0.0), sequence_number_(0) {
}

OrderBook::OrderBook(const std::string& symbol) 
    : symbol_(symbol), last_price_(0.0), best_bid_(0.0), best_ask_(0.0), sequence_number_(0) {
}

OrderBook::OrderBook(const OrderBook& other)
    : symbol_(other.symbol_), bids_(other.bids_), asks_(other.asks_),
      last_price_(other.last_price_), best_bid_(other.best_bid_), best_ask_(other.best_ask_),
      sequence_number_(other.sequence_number_), price_callback_(other.price_callback_) {}

OrderBook::OrderBook(OrderBook&& other) noexcept
    : symbol_(std::move(other.symbol_)), bids_(std::move(other.bids_)), asks_(std::move(other.asks_)),
      last_price_(other.last_price_), best_bid_(other.best_bid_), best_ask_(other.best_ask_),
      sequence_number_(other.sequence_number_), price_callback_(std::move(other.price_callback_)) {}

OrderBook& OrderBook::operator=(const OrderBook& other) {
    if (this != &other) {
        symbol_ = other.symbol_;
        bids_ = other.bids_;
        asks_ = other.asks_;
        last_price_ = other.last_price_;
        best_bid_ = other.best_bid_;
        best_ask_ = other.best_ask_;
        sequence_number_ = other.sequence_number_;
        price_callback_ = other.price_callback_;
    }
    return *this;
}

OrderBook& OrderBook::operator=(OrderBook&& other) noexcept {
    if (this != &other) {
        symbol_ = std::move(other.symbol_);
        bids_ = std::move(other.bids_);
        asks_ = std::move(other.asks_);
        last_price_ = other.last_price_;
        best_bid_ = other.best_bid_;
        best_ask_ = other.best_ask_;
        sequence_number_ = other.sequence_number_;
        price_callback_ = std::move(other.price_callback_);
    }
    return *this;
}

std::string OrderBook::get_symbol() const {
    return symbol_;
}

void OrderBook::add_order(const Order& order) {
    std::lock_guard<std::mutex> lock(book_mutex_);
    
    Order new_order = order;
    new_order.id = ++sequence_number_;
    new_order.timestamp = std::chrono::high_resolution_clock::now();
    
    if (order.side == OrderSide::BUY) {
        auto& level = bids_[order.price];
        level.price = order.price;
        level.orders.push_back(new_order);
        level.total_quantity += order.quantity;
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        level.orders.push_back(new_order);
        level.total_quantity += order.quantity;
    }
    
    update_best_prices();
    notify_price_update();
}

void OrderBook::cancel_order(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(book_mutex_);
    
    // Search in bids
    for (auto& [price, level] : bids_) {
        auto it = std::find_if(level.orders.begin(), level.orders.end(),
                              [order_id](const Order& order) { return order.id == order_id; });
        if (it != level.orders.end()) {
            level.total_quantity -= it->quantity;
            level.orders.erase(it);
            if (level.orders.empty()) {
                bids_.erase(price);
            }
            update_best_prices();
            return;
        }
    }
    
    // Search in asks
    for (auto& [price, level] : asks_) {
        auto it = std::find_if(level.orders.begin(), level.orders.end(),
                              [order_id](const Order& order) { return order.id == order_id; });
        if (it != level.orders.end()) {
            level.total_quantity -= it->quantity;
            level.orders.erase(it);
            if (level.orders.empty()) {
                asks_.erase(price);
            }
            update_best_prices();
            return;
        }
    }
}

void OrderBook::modify_order(uint64_t order_id, double new_price, uint32_t new_quantity) {
    std::lock_guard<std::mutex> lock(book_mutex_);
    
    // Find and remove the old order
    Order old_order;
    bool found = false;
    
    // Search in bids
    for (auto& [price, level] : bids_) {
        auto it = std::find_if(level.orders.begin(), level.orders.end(),
                              [order_id](const Order& order) { return order.id == order_id; });
        if (it != level.orders.end()) {
            old_order = *it;
            level.total_quantity -= it->quantity;
            level.orders.erase(it);
            if (level.orders.empty()) {
                bids_.erase(price);
            }
            found = true;
            break;
        }
    }
    
    // Search in asks
    if (!found) {
        for (auto& [price, level] : asks_) {
            auto it = std::find_if(level.orders.begin(), level.orders.end(),
                                  [order_id](const Order& order) { return order.id == order_id; });
            if (it != level.orders.end()) {
                old_order = *it;
                level.total_quantity -= it->quantity;
                level.orders.erase(it);
                if (level.orders.empty()) {
                    asks_.erase(price);
                }
                found = true;
                break;
            }
        }
    }
    
    if (found) {
        // Add the modified order
        old_order.price = new_price;
        old_order.quantity = new_quantity;
        add_order(old_order);
    }
}

double OrderBook::get_best_bid() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    return best_bid_;
}

double OrderBook::get_best_ask() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    return best_ask_;
}

double OrderBook::get_mid_price() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    if (best_bid_ > 0 && best_ask_ > 0) {
        return (best_bid_ + best_ask_) / 2.0;
    }
    return last_price_;
}

double OrderBook::get_spread() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    if (best_bid_ > 0 && best_ask_ > 0) {
        return best_ask_ - best_bid_;
    }
    return 0.0;
}

double OrderBook::get_last_price() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    return last_price_;
}

std::vector<PriceLevel> OrderBook::get_bid_levels(uint32_t depth) const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    std::vector<PriceLevel> levels;
    
    uint32_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= depth) break;
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

std::vector<PriceLevel> OrderBook::get_ask_levels(uint32_t depth) const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    std::vector<PriceLevel> levels;
    
    uint32_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= depth) break;
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

void OrderBook::set_price_update_callback(std::function<void(const std::string&, double, double)> callback) {
    price_callback_ = callback;
}

void OrderBook::update_best_prices() {
    if (!bids_.empty()) {
        best_bid_ = bids_.begin()->first;
    } else {
        best_bid_ = 0.0;
    }
    
    if (!asks_.empty()) {
        best_ask_ = asks_.begin()->first;
    } else {
        best_ask_ = 0.0;
    }
    
    // Update last price if we have both bid and ask
    if (best_bid_ > 0 && best_ask_ > 0) {
        last_price_ = (best_bid_ + best_ask_) / 2.0;
    }
}

void OrderBook::notify_price_update() {
    if (price_callback_) {
        price_callback_(symbol_, best_bid_, best_ask_);
    }
}

void OrderBook::set_last_price(double price) {
    std::lock_guard<std::mutex> lock(book_mutex_);
    last_price_ = price;
    update_best_prices();
    notify_price_update();
}

void OrderBook::clear_book() {
    std::lock_guard<std::mutex> lock(book_mutex_);
    bids_.clear();
    asks_.clear();
    update_best_prices();
}

void OrderBook::add_limit_order(double price, uint32_t quantity, OrderSide side) {
    std::lock_guard<std::mutex> lock(book_mutex_);
    Order synthetic;
    synthetic.id = 0; // 0 for synthetic/test
    synthetic.symbol = symbol_;
    synthetic.side = side;
    synthetic.type = OrderType::LIMIT;
    synthetic.price = price;
    synthetic.quantity = quantity;
    synthetic.filled_quantity = 0;
    synthetic.status = OrderStatus::PENDING;
    synthetic.timestamp = std::chrono::high_resolution_clock::now();
    synthetic.trader_id = "SIM";
    if (side == OrderSide::BUY) {
        auto& level = bids_[price];
        level.price = price;
        level.orders.push_back(synthetic);
        level.total_quantity += quantity;
    } else {
        auto& level = asks_[price];
        level.price = price;
        level.orders.push_back(synthetic);
        level.total_quantity += quantity;
    }
    update_best_prices();
}

// MarketDataFeed implementation
MarketDataFeed::MarketDataFeed() : running_(false) {
}

MarketDataFeed::~MarketDataFeed() {
    stop();
}

void MarketDataFeed::start() {
    if (!running_) {
        running_ = true;
        feed_thread_ = std::thread(&MarketDataFeed::feed_loop, this);
    }
}

void MarketDataFeed::stop() {
    if (running_) {
        running_ = false;
        if (feed_thread_.joinable()) {
            feed_thread_.join();
        }
    }
}

void MarketDataFeed::add_symbol(const std::string& symbol, double initial_price) {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    order_books_.emplace(symbol, OrderBook(symbol));
    
    // Set initial price
    Order initial_order;
    initial_order.symbol = symbol;
    initial_order.side = OrderSide::BUY;
    initial_order.type = OrderType::LIMIT;
    initial_order.price = initial_price;
    initial_order.quantity = 1000;
    initial_order.trader_id = "MARKET_MAKER";
    
    order_books_[symbol].add_order(initial_order);
    
    initial_order.side = OrderSide::SELL;
    initial_order.price = initial_price + 0.01;
    order_books_[symbol].add_order(initial_order);
}

OrderBook& MarketDataFeed::get_order_book(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    return order_books_[symbol];
}

const OrderBook& MarketDataFeed::get_order_book(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    return order_books_.at(symbol);
}

void MarketDataFeed::set_price_callback(std::function<void(const std::string&, double, double)> callback) {
    price_callback_ = callback;
}

void MarketDataFeed::set_order_callback(std::function<void(const Order&)> callback) {
    order_callback_ = callback;
}

void MarketDataFeed::feed_loop() {
    while (running_) {
        generate_market_data();
        simulate_market_microstructure();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void MarketDataFeed::generate_market_data() {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<> price_change(0.0, 0.001);
    static std::uniform_int_distribution<> order_side(0, 1);
    static std::uniform_int_distribution<> order_size(100, 1000);
    
    for (auto& [symbol, order_book] : order_books_) {
        double current_price = order_book.get_mid_price();
        if (current_price <= 0) continue;
        
        // Generate price movement
        double price_change_val = price_change(gen);
        double new_price = current_price * (1.0 + price_change_val);
        
        // Generate random orders
        if (std::uniform_real_distribution<>(0.0, 1.0)(gen) < 0.3) {
            Order order;
            order.symbol = symbol;
            order.side = order_side(gen) == 0 ? OrderSide::BUY : OrderSide::SELL;
            order.type = OrderType::LIMIT;
            order.price = new_price;
            order.quantity = order_size(gen);
            order.trader_id = "MARKET_MAKER";
            
            order_book.add_order(order);
            
            if (order_callback_) {
                order_callback_(order);
            }
        }
        
        // Notify price updates
        if (price_callback_) {
            price_callback_(symbol, order_book.get_best_bid(), order_book.get_best_ask());
        }
    }
}

void MarketDataFeed::simulate_market_microstructure() {
    // Simulate market microstructure effects like bid-ask spread dynamics
    // and order book depth changes
    std::lock_guard<std::mutex> lock(feed_mutex_);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> spread_change(-0.0001, 0.0001);
    
    for (auto& [symbol, order_book] : order_books_) {
        double current_spread = order_book.get_spread();
        if (current_spread <= 0) continue;
        
        // Simulate spread changes
        double spread_adjustment = spread_change(gen);
        double new_spread = std::max(0.001, current_spread + spread_adjustment);
        
        // Adjust order book to reflect new spread
        double mid_price = order_book.get_mid_price();
        if (mid_price > 0) {
            double new_bid = mid_price - new_spread / 2.0;
            double new_ask = mid_price + new_spread / 2.0;
            
            // This is a simplified implementation - in a real system,
            // you would modify existing orders or add new ones
        }
    }
}

void MarketDataFeed::update_price(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    auto it = order_books_.find(symbol);
    if (it != order_books_.end()) {
        it->second.set_last_price(price);
    }
}

} // namespace velocity 