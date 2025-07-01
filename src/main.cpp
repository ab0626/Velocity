#include "../include/velocity_engine.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace velocity;

void print_banner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    VELOCITY HFT SIMULATOR                    ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║  High-Frequency Trading Market Simulator                     ║\n";
    std::cout << "║  Built with C++ for maximum performance                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void print_status(const VelocityEngine& engine) {
    std::cout << "\n=== VELOCITY STATUS ===\n";
    std::cout << "Engine Running: " << (engine.is_running() ? "YES" : "NO") << "\n";
    std::cout << "Dashboard Running: " << (engine.is_dashboard_running() ? "YES" : "NO") << "\n";
    
    auto symbols = engine.get_symbols();
    std::cout << "Active Symbols: ";
    for (const auto& symbol : symbols) {
        std::cout << symbol << " ";
    }
    std::cout << "\n";
    
    auto strategy_names = engine.get_strategy_names();
    std::cout << "Active Strategies: ";
    for (const auto& name : strategy_names) {
        std::cout << name << " ";
    }
    std::cout << "\n";
    
    auto metrics = engine.get_performance_metrics();
    std::cout << "Total P&L: $" << std::fixed << std::setprecision(2) << metrics.total_pnl << "\n";
    std::cout << "Total Trades: " << metrics.total_trades << "\n";
    std::cout << "Win Rate: " << std::fixed << std::setprecision(2) << (metrics.win_rate * 100) << "%\n";
    std::cout << "Sharpe Ratio: " << std::fixed << std::setprecision(3) << metrics.sharpe_ratio << "\n";
    std::cout << "Max Drawdown: " << std::fixed << std::setprecision(2) << (metrics.max_drawdown * 100) << "%\n";
    std::cout << "=====================\n\n";
}

void print_order_book(const OrderBook& book, const std::string& symbol) {
    std::cout << "\n=== ORDER BOOK: " << symbol << " ===\n";
    std::cout << "Best Bid: $" << std::fixed << std::setprecision(4) << book.get_best_bid() << "\n";
    std::cout << "Best Ask: $" << std::fixed << std::setprecision(4) << book.get_best_ask() << "\n";
    std::cout << "Mid Price: $" << std::fixed << std::setprecision(4) << book.get_mid_price() << "\n";
    std::cout << "Spread: $" << std::fixed << std::setprecision(4) << book.get_spread() << "\n";
    
    auto bid_levels = book.get_bid_levels(5);
    auto ask_levels = book.get_ask_levels(5);
    
    std::cout << "\nAsks:\n";
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it) {
        std::cout << "  $" << std::fixed << std::setprecision(4) << it->price 
                  << " (" << it->total_quantity << ")\n";
    }
    
    std::cout << "\nBids:\n";
    for (const auto& level : bid_levels) {
        std::cout << "  $" << std::fixed << std::setprecision(4) << level.price 
                  << " (" << level.total_quantity << ")\n";
    }
    std::cout << "========================\n\n";
}

void print_positions(const std::vector<Position>& positions) {
    std::cout << "\n=== POSITIONS ===\n";
    if (positions.empty()) {
        std::cout << "No open positions\n";
    } else {
        for (const auto& pos : positions) {
            std::cout << pos.symbol << ": " << pos.quantity 
                      << " @ $" << std::fixed << std::setprecision(4) << pos.avg_price
                      << " (P&L: $" << std::fixed << std::setprecision(2) 
                      << (pos.realized_pnl + pos.unrealized_pnl) << ")\n";
        }
    }
    std::cout << "================\n\n";
}

int main() {
    print_banner();
    
    try {
        // Configure the Velocity engine
        VelocityConfig config;
        config.symbols = {"AAPL", "GOOGL", "MSFT", "TSLA", "AMZN"};
        config.initial_prices = {
            {"AAPL", 150.0},
            {"GOOGL", 2800.0},
            {"MSFT", 300.0},
            {"TSLA", 800.0},
            {"AMZN", 3300.0}
        };
        config.volatility_multiplier = 1.5;
        config.market_data_frequency_ms = 50;
        config.max_order_size = 10000;
        config.max_position_value = 1000000.0;
        config.max_daily_loss = 50000.0;
        config.enable_logging = true;
        config.log_directory = "./logs";
        config.enable_dashboard = true;
        config.dashboard_port = 8080;
        
        // Create and initialize the engine
        std::cout << "Initializing Velocity HFT Simulator...\n";
        VelocityEngine engine(config);
        
        // Add symbols
        for (const auto& [symbol, price] : config.initial_prices) {
            engine.add_symbol(symbol, price);
            std::cout << "Added symbol: " << symbol << " @ $" << price << "\n";
        }
        
        // Initialize the engine
        engine.initialize();
        
        // Add trading strategies for active trading
        std::cout << "Adding trading strategies...\n";
        
        // Add Market Making Strategy with aggressive settings
        std::map<std::string, std::string> mm_params = {
            {"spread_multiplier", "0.5"},  // Tighter spreads
            {"base_quantity", "1000"},     // Larger quantities
            {"max_position", "10000"},     // Larger position limits
            {"refresh_interval_ms", "500"} // More frequent updates
        };
        engine.add_strategy("market_making", "MM_Strategy", "MM_TRADER_01", mm_params);
        std::cout << "Added Market Making Strategy\n";
        
        // Add Momentum Strategy with aggressive settings
        std::map<std::string, std::string> momentum_params = {
            {"lookback_period", "5"},      // Shorter lookback
            {"momentum_threshold", "0.001"}, // Lower threshold
            {"position_size", "2000"},     // Larger position size
            {"max_positions", "5"}         // More positions
        };
        engine.add_strategy("momentum", "Momentum_Strategy", "MOMENTUM_TRADER_01", momentum_params);
        std::cout << "Added Momentum Strategy\n";
        
        // Add a simple market order strategy to ensure trades
        std::map<std::string, std::string> market_params = {
            {"order_interval_ms", "2000"},  // Place orders every 2 seconds
            {"order_size", "500"},          // Order size
            {"max_orders", "10"}            // Maximum number of orders
        };
        engine.add_strategy("market_orders", "Market_Order_Strategy", "MARKET_TRADER_01", market_params);
        std::cout << "Added Market Order Strategy\n";
        
        // Start the engine
        std::cout << "Starting Velocity engine...\n";
        engine.start();
        
        // Start the web dashboard
        std::cout << "Starting web dashboard on port " << config.dashboard_port << "...\n";
        engine.start_dashboard(config.dashboard_port);
        
        std::cout << "Velocity HFT Simulator is now running!\n";
        std::cout << "Web dashboard available at: http://localhost:" << config.dashboard_port << "\n";
        std::cout << "Press Ctrl+C to stop the simulator\n\n";
        
        // Main simulation loop
        int iteration = 0;
        while (engine.is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (iteration % 12 == 0) { // Print status every minute
                print_status(engine);
                
                // Print order book for first symbol
                auto symbols = engine.get_symbols();
                if (!symbols.empty()) {
                    print_order_book(engine.get_order_book(symbols[0]), symbols[0]);
                }
                
                // Print positions
                print_positions(engine.get_positions());
            }
            
            iteration++;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nVelocity HFT Simulator stopped.\n";
    return 0;
}

// Example usage functions for different scenarios
void run_market_making_example() {
    std::cout << "\n=== MARKET MAKING EXAMPLE ===\n";
    
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL"};
    config.initial_prices = {{"AAPL", 150.0}, {"GOOGL", 2800.0}};
    config.market_data_frequency_ms = 100;
    
    VelocityEngine engine(config);
    engine.initialize();
    
    // Add market making strategy
    std::map<std::string, std::string> mm_params = {
        {"spread_multiplier", "1.5"},
        {"base_quantity", "1000"},
        {"max_position", "10000"}
    };
    engine.add_strategy("market_making", "MM_Strategy", "MM_TRADER_01", mm_params);
    
    engine.start();
    engine.start_dashboard(8081);
    
    std::cout << "Market making example running on port 8081\n";
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    engine.stop();
}

void run_statistical_arbitrage_example() {
    std::cout << "\n=== STATISTICAL ARBITRAGE EXAMPLE ===\n";
    
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL"};
    config.initial_prices = {{"AAPL", 150.0}, {"GOOGL", 2800.0}};
    config.market_data_frequency_ms = 50;
    
    VelocityEngine engine(config);
    engine.initialize();
    
    // Add statistical arbitrage strategy
    std::map<std::string, std::string> stat_arb_params = {
        {"z_score_threshold", "2.0"},
        {"lookback_period", "100"},
        {"position_size", "5000"}
    };
    engine.add_strategy("stat_arb", "StatArb_Strategy", "STAT_ARB_01", stat_arb_params);
    
    engine.start();
    engine.start_dashboard(8082);
    
    std::cout << "Statistical arbitrage example running on port 8082\n";
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    engine.stop();
}

void run_momentum_trading_example() {
    std::cout << "\n=== MOMENTUM TRADING EXAMPLE ===\n";
    
    VelocityConfig config;
    config.symbols = {"TSLA", "AMZN"};
    config.initial_prices = {{"TSLA", 800.0}, {"AMZN", 3300.0}};
    config.market_data_frequency_ms = 75;
    
    VelocityEngine engine(config);
    engine.initialize();
    
    // Add momentum strategy
    std::map<std::string, std::string> momentum_params = {
        {"short_window", "10"},
        {"long_window", "30"},
        {"momentum_threshold", "0.02"},
        {"position_size", "2000"}
    };
    engine.add_strategy("momentum", "Momentum_Strategy", "MOMENTUM_01", momentum_params);
    
    engine.start();
    engine.start_dashboard(8083);
    
    std::cout << "Momentum trading example running on port 8083\n";
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    engine.stop();
} 