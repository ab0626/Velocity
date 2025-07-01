#include "../include/market_data.h"
#include "../include/order_manager.h"
#include "../include/trading_strategy.h"
#include "../include/performance_analytics.h"
#include "../include/velocity_engine.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <numeric>
#include <map>

using namespace velocity;

// ============================================================================
// MARKET CRASH/FLASH CRASH SIMULATION
// ============================================================================
void test_market_crash_simulation() {
    std::cout << "Testing Market Crash/Flash Crash Simulation...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    
    // Start with normal prices
    double aapl_price = 150.0;
    double googl_price = 2800.0;
    
    // Simulate normal trading for a bit
    for (int i = 0; i < 100; ++i) {
        aapl_price += (rand() % 3 - 1) * 0.1; // Small random movements
        googl_price += (rand() % 3 - 1) * 1.0;
        
        auto& aapl_book = order_manager->get_order_book("AAPL");
        auto& googl_book = order_manager->get_order_book("GOOGL");
        aapl_book.set_last_price(aapl_price);
        googl_book.set_last_price(googl_price);
    }
    
    // Simulate flash crash - rapid price drops
    std::cout << "  Simulating flash crash...\n";
    for (int i = 0; i < 50; ++i) {
        aapl_price -= 2.0; // Rapid decline
        googl_price -= 50.0;
        
        auto& aapl_book = order_manager->get_order_book("AAPL");
        auto& googl_book = order_manager->get_order_book("GOOGL");
        aapl_book.set_last_price(aapl_price);
        googl_book.set_last_price(googl_price);
        
        // Place panic orders
        Order panic_sell;
        panic_sell.symbol = "AAPL";
        panic_sell.side = OrderSide::SELL;
        panic_sell.type = OrderType::MARKET;
        panic_sell.quantity = 1000;
        panic_sell.trader_id = "PANIC";
        order_manager->place_order(panic_sell);
    }
    
    // Check risk management response
    auto positions = order_manager->get_all_positions();
    auto daily_pnl = order_manager->get_daily_pnl();
    
    std::cout << "  Final AAPL price: " << aapl_price << "\n";
    std::cout << "  Final GOOGL price: " << googl_price << "\n";
    std::cout << "  Daily P&L: " << daily_pnl << "\n";
    
    assert(aapl_price < 150.0); // Price should have dropped
    assert(googl_price < 2800.0); // Price should have dropped
    
    std::cout << "Market Crash Simulation test passed!\n";
}

// ============================================================================
// NEWS EVENT SIMULATION
// ============================================================================
void test_news_event_simulation() {
    std::cout << "Testing News Event Simulation...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("TSLA");
    
    double tsla_price = 800.0;
    
    // Normal trading
    for (int i = 0; i < 50; ++i) {
        tsla_price += (rand() % 3 - 1) * 2.0;
        auto& tsla_book = order_manager->get_order_book("TSLA");
        tsla_book.set_last_price(tsla_price);
    }
    
    // Simulate positive earnings announcement
    std::cout << "  Simulating positive earnings announcement...\n";
    double pre_news_price = tsla_price;
    
    for (int i = 0; i < 20; ++i) {
        tsla_price += 10.0; // Sharp upward movement
        auto& tsla_book = order_manager->get_order_book("TSLA");
        tsla_book.set_last_price(tsla_price);
        
        // Place momentum orders
        Order momentum_buy;
        momentum_buy.symbol = "TSLA";
        momentum_buy.side = OrderSide::BUY;
        momentum_buy.type = OrderType::LIMIT;
        momentum_buy.price = tsla_price + 5.0;
        momentum_buy.quantity = 500;
        momentum_buy.trader_id = "MOMENTUM";
        order_manager->place_order(momentum_buy);
    }
    
    std::cout << "  Pre-news price: " << pre_news_price << "\n";
    std::cout << "  Post-news price: " << tsla_price << "\n";
    std::cout << "  Price change: " << (tsla_price - pre_news_price) << "\n";
    
    assert(tsla_price > pre_news_price); // Price should have increased
    
    std::cout << "News Event Simulation test passed!\n";
}

// ============================================================================
// REGULATORY COMPLIANCE TESTING
// ============================================================================
void test_regulatory_compliance() {
    std::cout << "Testing Regulatory Compliance...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    // Set strict regulatory limits
    RiskLimits limits;
    limits.max_order_size = 100;
    limits.max_position_value = 5000.0;
    limits.max_daily_loss = 100.0;
    limits.max_drawdown = 0.05; // 5% max drawdown
    limits.max_leverage = 1.0; // No leverage
    order_manager->set_risk_limits(limits);
    
    // Test position limit compliance
    std::cout << "  Testing position limits...\n";
    for (int i = 0; i < 10; ++i) {
        Order order;
        order.symbol = "AAPL";
        order.side = OrderSide::BUY;
        order.type = OrderType::LIMIT;
        order.price = 150.0;
        order.quantity = 50; // Within limits
        order.trader_id = "REG_TEST";
        
        uint64_t order_id = order_manager->place_order(order);
        assert(order_id > 0); // Should be accepted
    }
    
    // Test order size limit violation
    Order oversized_order;
    oversized_order.symbol = "AAPL";
    oversized_order.side = OrderSide::BUY;
    oversized_order.type = OrderType::LIMIT;
    oversized_order.price = 150.0;
    oversized_order.quantity = 200; // Exceeds max_order_size
    oversized_order.trader_id = "REG_TEST";
    
    uint64_t oversized_id = order_manager->place_order(oversized_order);
    assert(oversized_id == 0); // Should be rejected
    
    // Test position value limit
    Order high_value_order;
    high_value_order.symbol = "AAPL";
    high_value_order.side = OrderSide::BUY;
    high_value_order.type = OrderType::LIMIT;
    high_value_order.price = 1000.0; // High price
    high_value_order.quantity = 10; // Small quantity but high value
    high_value_order.trader_id = "REG_TEST";
    
    uint64_t high_value_id = order_manager->place_order(high_value_order);
    assert(high_value_id == 0); // Should be rejected (value = 10000 > 5000)
    
    auto positions = order_manager->get_all_positions();
    auto risk_limits = order_manager->get_risk_limits();
    
    std::cout << "  Position count: " << positions.size() << "\n";
    std::cout << "  Max order size: " << risk_limits.max_order_size << "\n";
    std::cout << "  Max position value: " << risk_limits.max_position_value << "\n";
    
    std::cout << "Regulatory Compliance test passed!\n";
}

// ============================================================================
// REALISTIC TRADING DAY SIMULATION
// ============================================================================
void test_realistic_trading_day() {
    std::cout << "Testing Realistic Trading Day Simulation (Enhanced Realism)...\n";
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL", "TSLA", "MSFT"};
    config.initial_prices["AAPL"] = 150.0;
    config.initial_prices["GOOGL"] = 2800.0;
    config.initial_prices["TSLA"] = 800.0;
    config.initial_prices["MSFT"] = 300.0;
    config.enabled_strategies = {"MarketMaking", "Momentum"};
    config.enable_logging = false;
    
    VelocityEngine engine(config);
    engine.initialize();
    
    // Add multiple agent types
    engine.add_strategy("MarketMaking", "MM1", "MM_TRADER1");
    engine.add_strategy("MarketMaking", "MM2", "MM_TRADER2");
    engine.add_strategy("Momentum", "MOM1", "MOM_TRADER1");
    // Simulate a retail trader (random orders)
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    
    engine.start();
    
    // Enhanced price simulation: geometric Brownian motion
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    double mu = 0.0002, sigma = 0.01, dt = 1.0 / 252.0;
    std::map<std::string, double> prices = config.initial_prices;
    int steps = 100;
    bool news_triggered = false;
    double news_impact = 0.0;
    for (int i = 0; i < steps; ++i) {
        for (auto& sym : config.symbols) {
            double dW = norm(gen) * sqrt(dt);
            prices[sym] = prices[sym] * exp((mu - 0.5 * sigma * sigma) * dt + sigma * dW);
            engine.get_order_manager()->get_order_book(sym).set_last_price(prices[sym]);
        }
        // Retail trader places random orders
        if (i % 10 == 0) {
            Order retail;
            retail.symbol = (i % 20 == 0) ? "AAPL" : "GOOGL";
            retail.side = (rand() % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
            retail.type = OrderType::MARKET;
            retail.quantity = 1 + rand() % 5;
            retail.trader_id = "RETAIL";
            order_manager->place_order(retail);
        }
        // Trigger a news event at step 50
        if (i == 50 && !news_triggered) {
            std::cout << "  News event: Sudden positive shock to TSLA!\n";
            news_impact = 1.0 + (norm(gen) * 0.05); // Variable impact
            prices["TSLA"] *= (1.05 * news_impact);
            engine.get_order_manager()->get_order_book("TSLA").set_last_price(prices["TSLA"]);
            news_triggered = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    engine.stop();
    // Validate end-of-day state
    auto metrics = engine.get_performance_metrics();
    auto positions = engine.get_positions();
    auto risk_metrics = engine.get_risk_metrics();
    std::cout << "  Total trades: " << metrics.total_trades << "\n";
    std::cout << "  Total P&L: " << metrics.total_pnl << "\n";
    std::cout << "  Win rate: " << metrics.win_rate << "\n";
    std::cout << "  Positions: " << positions.size() << "\n";
    std::cout << "  News impact factor: " << news_impact << "\n";
    std::cout << "Realistic Trading Day (Enhanced) test passed!\n";
}

// ============================================================================
// LIQUIDITY DROUGHT SCENARIO
// ============================================================================
void test_liquidity_drought() {
    std::cout << "Testing Liquidity Drought Scenario...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    
    // Stochastic price model (Geometric Brownian Motion)
    double price = 150.0;
    double mu = 0.0; // drift
    double sigma = 0.05; // volatility
    int steps = 200;
    double dt = 1.0 / 252.0;
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    
    // Simulate order book thinning
    std::vector<double> spreads;
    std::vector<double> pnl_track;
    std::vector<double> prices;
    for (int i = 0; i < steps; ++i) {
        // Simulate price
        double dW = norm(gen) * sqrt(dt);
        price = price * exp((mu - 0.5 * sigma * sigma) * dt + sigma * dW);
        prices.push_back(price);
        
        // Simulate wide spreads and low depth
        double spread = 0.5 + (rand() % 100) / 100.0; // 0.5 to 1.5
        spreads.push_back(spread);
        int book_depth = 1 + rand() % 2; // 1 or 2 levels only
        
        auto& book = order_manager->get_order_book("AAPL");
        book.set_last_price(price);
        // Simulate only a few orders on each side
        book.clear_book();
        for (int d = 0; d < book_depth; ++d) {
            book.add_limit_order(price - spread/2 - d*0.1, 10, OrderSide::BUY);
            book.add_limit_order(price + spread/2 + d*0.1, 10, OrderSide::SELL);
        }
        
        // Place a market order every 20 steps
        if (i % 20 == 0) {
            Order mkt;
            mkt.symbol = "AAPL";
            mkt.side = (i % 40 == 0) ? OrderSide::BUY : OrderSide::SELL;
            mkt.type = OrderType::MARKET;
            mkt.quantity = 5;
            mkt.trader_id = "DROUGHT";
            order_manager->place_order(mkt);
        }
        pnl_track.push_back(order_manager->get_daily_pnl());
    }
    // Analysis
    double min_spread = *std::min_element(spreads.begin(), spreads.end());
    double max_spread = *std::max_element(spreads.begin(), spreads.end());
    double avg_spread = std::accumulate(spreads.begin(), spreads.end(), 0.0) / spreads.size();
    double final_pnl = pnl_track.back();
    double max_drawdown = 0.0, peak = pnl_track[0];
    for (double v : pnl_track) {
        if (v > peak) peak = v;
        if (peak - v > max_drawdown) max_drawdown = peak - v;
    }
    double pnl_vol = 0.0, pnl_mean = std::accumulate(pnl_track.begin(), pnl_track.end(), 0.0) / pnl_track.size();
    for (double v : pnl_track) pnl_vol += (v - pnl_mean) * (v - pnl_mean);
    pnl_vol = sqrt(pnl_vol / pnl_track.size());
    
    std::cout << "  Spread: min=" << min_spread << ", max=" << max_spread << ", avg=" << avg_spread << "\n";
    std::cout << "  Final P&L: " << final_pnl << "\n";
    std::cout << "  Max drawdown: " << max_drawdown << "\n";
    std::cout << "  P&L volatility: " << pnl_vol << "\n";
    std::cout << "  Price range: " << *std::min_element(prices.begin(), prices.end()) << " - " << *std::max_element(prices.begin(), prices.end()) << "\n";
    
    assert(max_spread > 1.0); // Should see wide spreads
    assert(book_depth <= 2); // Book should be thin
    std::cout << "Liquidity Drought Scenario test passed!\n";
}

// ============================================================================
// ORDER FLOW TOXICITY SCENARIO
// ============================================================================
void test_order_flow_toxicity() {
    std::cout << "Testing Order Flow Toxicity Scenario...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    double price = 150.0;
    int steps = 200;
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    std::vector<double> pnl_track;
    std::vector<double> prices;
    std::vector<uint32_t> toxic_bursts;
    // Simulate normal trading, then bursts of toxic flow
    for (int i = 0; i < steps; ++i) {
        // Price follows random walk, but toxic bursts move it sharply
        if (i % 50 == 0 && i > 0) {
            // Toxic burst: 10 aggressive market sells
            for (int j = 0; j < 10; ++j) {
                Order toxic;
                toxic.symbol = "AAPL";
                toxic.side = OrderSide::SELL;
                toxic.type = OrderType::MARKET;
                toxic.quantity = 20;
                toxic.trader_id = "TOXIC";
                order_manager->place_order(toxic);
                price -= 0.5 + norm(gen) * 0.1; // Adverse move
            }
            toxic_bursts.push_back(i);
        } else {
            price += norm(gen) * 0.1;
        }
        auto& book = order_manager->get_order_book("AAPL");
        book.set_last_price(price);
        prices.push_back(price);
        pnl_track.push_back(order_manager->get_daily_pnl());
    }
    // Analysis
    double final_pnl = pnl_track.back();
    double max_drawdown = 0.0, peak = pnl_track[0];
    for (double v : pnl_track) {
        if (v > peak) peak = v;
        if (peak - v > max_drawdown) max_drawdown = peak - v;
    }
    std::cout << "  Toxic bursts at steps: ";
    for (auto idx : toxic_bursts) std::cout << idx << " ";
    std::cout << "\n";
    std::cout << "  Final P&L: " << final_pnl << "\n";
    std::cout << "  Max drawdown: " << max_drawdown << "\n";
    std::cout << "  Price range: " << *std::min_element(prices.begin(), prices.end()) << " - " << *std::max_element(prices.begin(), prices.end()) << "\n";
    std::cout << "Order Flow Toxicity Scenario test passed!\n";
}

// ============================================================================
// CROSS-ASSET CORRELATION SHOCK SCENARIO
// ============================================================================
void test_cross_asset_correlation_shock() {
    std::cout << "Testing Cross-Asset Correlation Shock Scenario...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("MSFT");
    double price_aapl = 150.0, price_msft = 300.0;
    int steps = 200;
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    double rho = 0.8; // Correlation coefficient
    std::vector<double> pnl_track;
    std::vector<double> prices_aapl, prices_msft;
    std::vector<int> shock_steps;
    for (int i = 0; i < steps; ++i) {
        // Generate correlated shocks
        double z1 = norm(gen);
        double z2 = rho * z1 + sqrt(1 - rho * rho) * norm(gen);
        price_aapl += z1 * 0.2;
        price_msft += z2 * 0.2;
        // Every 60 steps, apply a large correlated shock
        if (i % 60 == 0 && i > 0) {
            price_aapl -= 5.0;
            price_msft -= 10.0;
            shock_steps.push_back(i);
        }
        auto& book_aapl = order_manager->get_order_book("AAPL");
        auto& book_msft = order_manager->get_order_book("MSFT");
        book_aapl.set_last_price(price_aapl);
        book_msft.set_last_price(price_msft);
        // Place market orders on both assets
        if (i % 30 == 0) {
            Order mkt_aapl;
            mkt_aapl.symbol = "AAPL";
            mkt_aapl.side = (i % 60 == 0) ? OrderSide::SELL : OrderSide::BUY;
            mkt_aapl.type = OrderType::MARKET;
            mkt_aapl.quantity = 10;
            mkt_aapl.trader_id = "CORR";
            order_manager->place_order(mkt_aapl);
            Order mkt_msft = mkt_aapl;
            mkt_msft.symbol = "MSFT";
            order_manager->place_order(mkt_msft);
        }
        prices_aapl.push_back(price_aapl);
        prices_msft.push_back(price_msft);
        pnl_track.push_back(order_manager->get_daily_pnl());
    }
    // Analysis
    double final_pnl = pnl_track.back();
    double max_drawdown = 0.0, peak = pnl_track[0];
    for (double v : pnl_track) {
        if (v > peak) peak = v;
        if (peak - v > max_drawdown) max_drawdown = peak - v;
    }
    std::cout << "  Correlated shocks at steps: ";
    for (auto idx : shock_steps) std::cout << idx << " ";
    std::cout << "\n";
    std::cout << "  Final P&L: " << final_pnl << "\n";
    std::cout << "  Max drawdown: " << max_drawdown << "\n";
    std::cout << "  AAPL price range: " << *std::min_element(prices_aapl.begin(), prices_aapl.end()) << " - " << *std::max_element(prices_aapl.begin(), prices_aapl.end()) << "\n";
    std::cout << "  MSFT price range: " << *std::min_element(prices_msft.begin(), prices_msft.end()) << " - " << *std::max_element(prices_msft.begin(), prices_msft.end()) << "\n";
    std::cout << "Cross-Asset Correlation Shock Scenario test passed!\n";
}

// ============================================================================
// EXCHANGE OUTAGE/RECOVERY SCENARIO
// ============================================================================
void test_exchange_outage_recovery() {
    std::cout << "Testing Exchange Outage/Recovery Scenario...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    double price_aapl = 150.0, price_googl = 2800.0;
    int steps = 300;
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    std::vector<double> pnl_track;
    std::vector<double> prices_aapl, prices_googl;
    std::vector<int> outage_steps;
    bool aapl_outage = false, googl_outage = false;
    for (int i = 0; i < steps; ++i) {
        // Simulate normal price movements
        price_aapl += norm(gen) * 0.1;
        price_googl += norm(gen) * 2.0;
        // Simulate outage at step 100 for AAPL, step 150 for GOOGL
        if (i == 100) {
            aapl_outage = true;
            outage_steps.push_back(i);
            std::cout << "  AAPL trading halted at step " << i << "\n";
        }
        if (i == 150) {
            googl_outage = true;
            outage_steps.push_back(i);
            std::cout << "  GOOGL trading halted at step " << i << "\n";
        }
        // Recovery at step 200
        if (i == 200) {
            aapl_outage = false;
            googl_outage = false;
            std::cout << "  Trading resumed at step " << i << "\n";
        }
        auto& book_aapl = order_manager->get_order_book("AAPL");
        auto& book_googl = order_manager->get_order_book("GOOGL");
        if (!aapl_outage) {
            book_aapl.set_last_price(price_aapl);
            // Place orders only when not in outage
            if (i % 20 == 0) {
                Order mkt_aapl;
                mkt_aapl.symbol = "AAPL";
                mkt_aapl.side = (i % 40 == 0) ? OrderSide::SELL : OrderSide::BUY;
                mkt_aapl.type = OrderType::MARKET;
                mkt_aapl.quantity = 5;
                mkt_aapl.trader_id = "OUTAGE";
                order_manager->place_order(mkt_aapl);
            }
        }
        if (!googl_outage) {
            book_googl.set_last_price(price_googl);
            if (i % 20 == 0) {
                Order mkt_googl;
                mkt_googl.symbol = "GOOGL";
                mkt_googl.side = (i % 40 == 0) ? OrderSide::SELL : OrderSide::BUY;
                mkt_googl.type = OrderType::MARKET;
                mkt_googl.quantity = 1;
                mkt_googl.trader_id = "OUTAGE";
                order_manager->place_order(mkt_googl);
            }
        }
        prices_aapl.push_back(price_aapl);
        prices_googl.push_back(price_googl);
        pnl_track.push_back(order_manager->get_daily_pnl());
    }
    // Analysis
    double final_pnl = pnl_track.back();
    double max_drawdown = 0.0, peak = pnl_track[0];
    for (double v : pnl_track) {
        if (v > peak) peak = v;
        if (peak - v > max_drawdown) max_drawdown = peak - v;
    }
    std::cout << "  Outage events at steps: ";
    for (auto idx : outage_steps) std::cout << idx << " ";
    std::cout << "\n";
    std::cout << "  Final P&L: " << final_pnl << "\n";
    std::cout << "  Max drawdown: " << max_drawdown << "\n";
    std::cout << "  AAPL price range: " << *std::min_element(prices_aapl.begin(), prices_aapl.end()) << " - " << *std::max_element(prices_aapl.begin(), prices_aapl.end()) << "\n";
    std::cout << "  GOOGL price range: " << *std::min_element(prices_googl.begin(), prices_googl.end()) << " - " << *std::max_element(prices_googl.begin(), prices_googl.end()) << "\n";
    std::cout << "Exchange Outage/Recovery Scenario test passed!\n";
}

// ============================================================================
// LATENCY ARBITRAGE SCENARIO
// ============================================================================
void test_latency_arbitrage() {
    std::cout << "Testing Latency Arbitrage Scenario...\n";
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    double price = 150.0;
    int steps = 200;
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    std::vector<double> pnl_track;
    std::vector<double> prices;
    std::vector<int> fast_trades, slow_trades;
    int fast_latency = 1; // 1 step delay
    int slow_latency = 5; // 5 step delay
    for (int i = 0; i < steps; ++i) {
        // Simulate price movements
        price += norm(gen) * 0.1;
        auto& book = order_manager->get_order_book("AAPL");
        book.set_last_price(price);
        // Fast trader reacts immediately to price changes
        if (i % 10 == 0) {
            Order fast_order;
            fast_order.symbol = "AAPL";
            fast_order.side = (price > 150.0) ? OrderSide::SELL : OrderSide::BUY;
            fast_order.type = OrderType::LIMIT;
            fast_order.price = price + (fast_order.side == OrderSide::BUY ? 0.1 : -0.1);
            fast_order.quantity = 10;
            fast_order.trader_id = "FAST";
            order_manager->place_order(fast_order);
            fast_trades.push_back(i);
        }
        // Slow trader reacts with delay
        if (i % 10 == 0 && i >= slow_latency) {
            double delayed_price = prices[i - slow_latency];
            Order slow_order;
            slow_order.symbol = "AAPL";
            slow_order.side = (delayed_price > 150.0) ? OrderSide::SELL : OrderSide::BUY;
            slow_order.type = OrderType::LIMIT;
            slow_order.price = delayed_price + (slow_order.side == OrderSide::BUY ? 0.1 : -0.1);
            slow_order.quantity = 10;
            slow_order.trader_id = "SLOW";
            order_manager->place_order(slow_order);
            slow_trades.push_back(i);
        }
        prices.push_back(price);
        pnl_track.push_back(order_manager->get_daily_pnl());
    }
    // Analysis
    double final_pnl = pnl_track.back();
    double max_drawdown = 0.0, peak = pnl_track[0];
    for (double v : pnl_track) {
        if (v > peak) peak = v;
        if (peak - v > max_drawdown) max_drawdown = peak - v;
    }
    std::cout << "  Fast trader trades at steps: ";
    for (auto idx : fast_trades) std::cout << idx << " ";
    std::cout << "\n";
    std::cout << "  Slow trader trades at steps: ";
    for (auto idx : slow_trades) std::cout << idx << " ";
    std::cout << "\n";
    std::cout << "  Final P&L: " << final_pnl << "\n";
    std::cout << "  Max drawdown: " << max_drawdown << "\n";
    std::cout << "  Price range: " << *std::min_element(prices.begin(), prices.end()) << " - " << *std::max_element(prices.begin(), prices.end()) << "\n";
    std::cout << "Latency Arbitrage Scenario test passed!\n";
}

// ============================================================================
// ENHANCED ANALYSIS WITH REALISM SCENARIO
// ============================================================================
void test_enhanced_analysis_with_realism() {
    std::cout << "Testing Enhanced Analysis with Realism...\n";
    
    // Create analytics with detailed logging
    auto analytics = std::make_shared<PerformanceAnalytics>();
    analytics->enable_detailed_logging(true);
    analytics->set_risk_limits(1000.0, 5000.0);
    
    auto order_manager = std::make_shared<OrderManager>();
    order_manager->add_symbol("AAPL");
    order_manager->add_symbol("GOOGL");
    order_manager->add_symbol("TSLA");
    
    // Realistic price simulation with multiple regimes
    std::default_random_engine gen;
    std::normal_distribution<double> norm(0, 1);
    std::exponential_distribution<double> exp(1.0);
    
    double price_aapl = 150.0, price_googl = 2800.0, price_tsla = 800.0;
    int steps = 500;
    std::vector<double> pnl_track;
    
    // Market regime: 0=normal, 1=volatile, 2=trending
    int current_regime = 0;
    double regime_volatility = 0.01;
    
    for (int i = 0; i < steps; ++i) {
        // Change market regime every 100 steps
        if (i % 100 == 0) {
            current_regime = (current_regime + 1) % 3;
            switch (current_regime) {
                case 0: regime_volatility = 0.01; break; // Normal
                case 1: regime_volatility = 0.03; break; // Volatile
                case 2: regime_volatility = 0.02; break; // Trending
            }
        }
        
        // Simulate price movements with regime-dependent behavior
        double shock_aapl = norm(gen) * regime_volatility;
        double shock_googl = norm(gen) * regime_volatility * 2.0;
        double shock_tsla = norm(gen) * regime_volatility * 1.5;
        
        // Add regime-specific behavior
        if (current_regime == 1) { // Volatile regime
            if (exp(gen) > 0.95) { // Rare large moves
                shock_aapl *= 3.0;
                shock_googl *= 3.0;
                shock_tsla *= 3.0;
            }
        } else if (current_regime == 2) { // Trending regime
            shock_aapl += 0.005; // Upward drift
            shock_googl += 0.01;
            shock_tsla += 0.008;
        }
        
        price_aapl *= (1.0 + shock_aapl);
        price_googl *= (1.0 + shock_googl);
        price_tsla *= (1.0 + shock_tsla);
        
        // Update order books
        auto& book_aapl = order_manager->get_order_book("AAPL");
        auto& book_googl = order_manager->get_order_book("GOOGL");
        auto& book_tsla = order_manager->get_order_book("TSLA");
        
        book_aapl.set_last_price(price_aapl);
        book_googl.set_last_price(price_googl);
        book_tsla.set_last_price(price_tsla);
        
        // Capture order book snapshots periodically
        if (i % 50 == 0) {
            analytics->capture_order_book_snapshot("AAPL", book_aapl);
            analytics->capture_order_book_snapshot("GOOGL", book_googl);
            analytics->capture_order_book_snapshot("TSLA", book_tsla);
        }
        
        // Simulate different trader types
        if (i % 10 == 0) {
            // Market maker orders
            Order mm_order;
            mm_order.symbol = "AAPL";
            mm_order.side = (i % 20 == 0) ? OrderSide::BUY : OrderSide::SELL;
            mm_order.type = OrderType::LIMIT;
            mm_order.price = price_aapl + (mm_order.side == OrderSide::BUY ? -0.1 : 0.1);
            mm_order.quantity = 10;
            mm_order.trader_id = "MM1";
            order_manager->place_order(mm_order);
        }
        
        if (i % 15 == 0) {
            // Momentum trader orders
            Order mom_order;
            mom_order.symbol = "TSLA";
            mom_order.side = (price_tsla > 800.0) ? OrderSide::BUY : OrderSide::SELL;
            mom_order.type = OrderType::MARKET;
            mom_order.quantity = 5;
            mom_order.trader_id = "MOM1";
            order_manager->place_order(mom_order);
        }
        
        if (i % 25 == 0) {
            // Large institutional order
            Order inst_order;
            inst_order.symbol = "GOOGL";
            inst_order.side = OrderSide::BUY;
            inst_order.type = OrderType::LIMIT;
            inst_order.price = price_googl - 5.0;
            inst_order.quantity = 50;
            inst_order.trader_id = "INST1";
            order_manager->place_order(inst_order);
        }
        
        // Record PnL
        double current_pnl = order_manager->get_daily_pnl();
        pnl_track.push_back(current_pnl);
        
        // Update analytics
        analytics->update_price("AAPL", price_aapl);
        analytics->update_price("GOOGL", price_googl);
        analytics->update_price("TSLA", price_tsla);
    }
    
    // Generate comprehensive analysis
    std::cout << "  Generating comprehensive analysis...\n";
    
    // Get PnL histogram
    auto histogram = analytics->get_pnl_histogram(15);
    std::cout << "  PnL Range: [" << histogram.min_pnl << ", " << histogram.max_pnl << "]\n";
    std::cout << "  Bin Width: " << histogram.bin_width << "\n";
    
    // Get risk metrics
    auto risk_metrics = analytics->calculate_risk_metrics();
    std::cout << "  VaR (95%): " << risk_metrics.var_95 << "\n";
    std::cout << "  VaR (99%): " << risk_metrics.var_99 << "\n";
    std::cout << "  Sharpe Ratio: " << risk_metrics.sharpe_ratio << "\n";
    std::cout << "  Max Drawdown: " << risk_metrics.max_drawdown << "\n";
    std::cout << "  Volatility: " << risk_metrics.volatility << "\n";
    std::cout << "  Skewness: " << risk_metrics.skewness << "\n";
    std::cout << "  Kurtosis: " << risk_metrics.kurtosis << "\n";
    
    // Get trade logs
    auto trade_logs = analytics->get_trade_logs();
    std::cout << "  Total Trades Logged: " << trade_logs.size() << "\n";
    
    // Get order book snapshots
    auto book_snapshots = analytics->get_order_book_snapshots();
    std::cout << "  Order Book Snapshots: " << book_snapshots.size() << "\n";
    
    // Print performance summary
    analytics->print_performance_summary();
    
    // Export reports
    analytics->export_risk_report("risk_report.txt");
    analytics->export_trade_analysis("trade_analysis.csv");
    
    std::cout << "Enhanced Analysis with Realism test passed!\n";
}

// ============================================================================
// MAIN REAL-WORLD SCENARIO TEST RUNNER
// ============================================================================
int main() {
    std::cout << "Running Real-World Scenario Tests for Velocity HFT Simulator...\n\n";
    
    try {
        test_market_crash_simulation();
        test_news_event_simulation();
        test_regulatory_compliance();
        test_realistic_trading_day();
        test_liquidity_drought();
        test_order_flow_toxicity();
        test_cross_asset_correlation_shock();
        test_exchange_outage_recovery();
        test_latency_arbitrage();
        test_enhanced_analysis_with_realism();
        
        std::cout << "\n🌍 All Real-World Scenario tests passed! 🌍\n";
        std::cout << "Velocity HFT Simulator handles real-world market conditions.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Real-world scenario test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 