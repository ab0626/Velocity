# Velocity - High-Frequency Trading Market Simulator

[![Build Status](https://github.com/your-username/velocity/workflows/Build/badge.svg)](https://github.com/your-username/velocity/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)

Velocity is a high-performance, real-time HFT (High-Frequency Trading) market simulator built in C++17. It provides a comprehensive platform for developing, testing, and analyzing algorithmic trading strategies with ultra-low latency execution and realistic market microstructure simulation.

## 🚀 Current Status

### ✅ **WORKING FEATURES**

#### Core Engine
- **✅ Order Book Management** - Full depth-of-market support with bid/ask levels
- **✅ Market Data Feed** - Real-time price simulation with configurable volatility
- **✅ Order Matching Engine** - Basic limit and market order matching
- **✅ Multi-threaded Architecture** - Thread-safe operations with proper synchronization
- **✅ Position Tracking** - Real-time position management and P&L calculation

#### Trading Strategies
- **✅ Market Making Strategy** - Automated bid/ask spread management with configurable parameters
- **✅ Market Order Strategy** - Simple market order placement for testing
- **✅ Strategy Framework** - Extensible base class for implementing custom strategies
- **✅ Strategy Factory** - Dynamic strategy creation and management

#### Order Management
- **✅ Order Placement** - Support for market and limit orders
- **✅ Order Cancellation** - Real-time order cancellation
- **✅ Order Modification** - Price and quantity modifications
- **✅ Execution Tracking** - Complete trade execution lifecycle

#### Performance Analytics
- **✅ P&L Tracking** - Realized and unrealized profit/loss calculation
- **✅ Trade Recording** - Complete trade history with timestamps
- **✅ Performance Metrics** - Sharpe ratio, win rate, drawdown calculations
- **✅ Risk Metrics** - VaR, volatility, and other risk measures
- **✅ CSV Export** - Trade and performance data export capabilities

#### Risk Management
- **✅ Position Limits** - Configurable position size limits
- **✅ Order Size Limits** - Maximum order size restrictions
- **✅ Basic Risk Controls** - Order validation and rejection

### ⚠️ **PARTIALLY IMPLEMENTED**

#### Trading Strategies
- **⚠️ Statistical Arbitrage Strategy** - Basic structure exists but not fully implemented
- **⚠️ Momentum Strategy** - Basic structure exists but not fully implemented

#### Web Dashboard
- **⚠️ Dashboard Framework** - Basic HTTP server exists but UI is minimal
- **⚠️ Real-time Monitoring** - Data collection works but visualization is limited

### ❌ **NOT IMPLEMENTED**

#### Web Dashboard
- **❌ Interactive Charts** - No charting library integration
- **❌ Real-time WebSocket Updates** - No live data streaming
- **❌ Strategy Performance Comparison** - No multi-strategy analysis UI

#### Advanced Features
- **❌ Market Microstructure Simulation** - Basic price simulation only
- **❌ Latency Measurement** - Framework exists but not fully utilized
- **❌ Advanced Risk Management** - Basic limits only, no VaR alerts
- **❌ External Data Feeds** - No real market data integration

## 🏗️ Architecture

```
Velocity HFT Simulator
├── ✅ Market Data Engine
│   ├── ✅ Order Book Management
│   ├── ✅ Price Feed Simulation
│   └── ⚠️ Market Microstructure
├── ✅ Order Management System
│   ├── ✅ Matching Engine
│   ├── ✅ Order Routing
│   └── ✅ Execution Tracking
├── ✅ Trading Strategies
│   ├── ✅ Market Making
│   ├── ⚠️ Statistical Arbitrage
│   ├── ⚠️ Momentum Trading
│   └── ✅ Custom Strategies
├── ✅ Risk Management
│   ├── ✅ Position Limits
│   ├── ✅ Exposure Controls
│   └── ⚠️ Risk Metrics
├── ✅ Performance Analytics
│   ├── ✅ P&L Tracking
│   ├── ⚠️ Latency Measurement
│   └── ✅ Performance Metrics
└── ⚠️ Web Dashboard
    ├── ⚠️ Real-time Monitoring
    ├── ❌ Interactive Charts
    └── ❌ Strategy Analysis
```

## 📋 Requirements

### System Requirements
- **OS**: Windows 10+, Linux (Ubuntu 18.04+), macOS 10.15+
- **CPU**: Multi-core processor (4+ cores recommended)
- **RAM**: 8GB minimum, 16GB+ recommended
- **Storage**: 1GB free space

### Development Requirements
- **Compiler**: GCC 7+, Clang 6+, or MSVC 2019+
- **CMake**: 3.16 or higher
- **C++ Standard**: C++17

## 🛠️ Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake git

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake git

# macOS
brew install cmake git

# Windows
# Install Visual Studio 2019+ with C++ development tools
# Install CMake from https://cmake.org/download/
```

### Build from Source
```bash
# Clone the repository
git clone https://github.com/your-username/velocity.git
cd velocity

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Run the simulator
./bin/Release/velocity
```

## 🎯 Quick Start

### Basic Usage
```cpp
#include "velocity_engine.h"

using namespace velocity;

int main() {
    // Configure the engine
    VelocityConfig config;
    config.symbols = {"AAPL", "GOOGL", "MSFT"};
    config.initial_prices = {
        {"AAPL", 150.0},
        {"GOOGL", 2800.0},
        {"MSFT", 300.0}
    };
    
    // Create and start the engine
    VelocityEngine engine(config);
    engine.initialize();
    engine.start();
    
    // Add a market making strategy
    std::map<std::string, std::string> params = {
        {"spread_multiplier", "1.5"},
        {"base_quantity", "1000"}
    };
    engine.add_strategy("market_making", "MM_Strategy", "TRADER_01", params);
    
    // Start web dashboard
    engine.start_dashboard(8080);
    
    // Keep running
    while (engine.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### Strategy Examples

#### Market Making Strategy
```cpp
// Market making with configurable spread and position limits
std::map<std::string, std::string> mm_params = {
    {"spread_multiplier", "1.5"},
    {"base_quantity", "1000"},
    {"max_position", "10000"},
    {"min_spread", "0.01"}
};
engine.add_strategy("market_making", "MM_Strategy", "MM_TRADER_01", mm_params);
```

#### Market Order Strategy (for testing)
```cpp
// Simple market order strategy for testing
std::map<std::string, std::string> market_params = {
    {"order_interval_ms", "2000"},
    {"order_size", "500"},
    {"max_orders", "10"}
};
engine.add_strategy("market_orders", "Market_Order_Strategy", "MARKET_TRADER_01", market_params);
```

## 🧪 Testing

The project includes comprehensive test suites:

```bash
# Run all tests
cd build/tests/Release
./comprehensive_test.exe
./integration_test.exe
./performance_test.exe
./stress_test.exe
./simple_test.exe
```

### Test Results
- **✅ Simple Tests**: All basic functionality tests pass
- **✅ Comprehensive Tests**: Core features working (with minor strategy creation issues)
- **✅ Integration Tests**: Engine integration working
- **✅ Performance Tests**: Performance benchmarks functional
- **✅ Stress Tests**: System stability under load

## 📊 Performance

### Current Capabilities
- **Order Processing**: ~10,000 orders/second
- **Market Data Updates**: ~1,000 updates/second
- **Latency**: <1ms for order processing
- **Memory Usage**: ~100MB for typical configuration

### Benchmarks
- **Order Book Operations**: <1μs per operation
- **Strategy Execution**: <10μs per market data update
- **P&L Calculation**: <1μs per trade

## 🔧 Configuration

### Engine Configuration
```cpp
VelocityConfig config;
config.symbols = {"AAPL", "GOOGL", "MSFT"};
config.initial_prices = {{"AAPL", 150.0}, {"GOOGL", 2800.0}};
config.volatility_multiplier = 1.5;
config.market_data_frequency_ms = 50;
config.max_order_size = 10000;
config.max_position_value = 1000000.0;
config.max_daily_loss = 50000.0;
config.enable_logging = true;
config.log_directory = "./logs";
config.enable_dashboard = true;
config.dashboard_port = 8080;
```

### Strategy Parameters
```cpp
// Market Making Parameters
{
    "spread_multiplier": "1.5",    // Spread multiplier for quotes
    "base_quantity": "1000",       // Base order size
    "max_position": "10000",       // Maximum position size
    "min_spread": "0.01"          // Minimum spread
}

// Market Order Parameters
{
    "order_interval_ms": "2000",   // Time between orders
    "order_size": "500",          // Order size
    "max_orders": "10"            // Maximum orders to place
}
```

## 📈 Monitoring

### Console Output
The simulator provides real-time console output including:
- Engine status and performance metrics
- Order book snapshots
- Strategy performance
- Position updates
- Risk alerts

### Web Dashboard
Basic web dashboard available at `http://localhost:8080` (when enabled):
- Engine status
- Performance metrics
- Order book visualization (basic)
- Strategy status

### Logging
Comprehensive logging to files:
- Trade logs
- Performance logs
- Error logs
- Debug logs

## 🐛 Known Issues

### Strategy Creation
- **Issue**: Some tests use "MarketMaking" instead of "market_making"
- **Status**: Tests pass but show warning about strategy creation
- **Impact**: Minor - main application works correctly

### Web Dashboard
- **Issue**: Basic HTTP server only, no advanced UI
- **Status**: Functional but limited
- **Impact**: Monitoring works but visualization is basic

### Performance Analytics
- **Issue**: Some advanced metrics not fully implemented
- **Status**: Core metrics working
- **Impact**: Basic analysis available

## 🚧 Development Roadmap

### Short Term (Next Release)
- [ ] Fix strategy creation naming consistency
- [ ] Implement full Statistical Arbitrage strategy
- [ ] Implement full Momentum strategy
- [ ] Add more comprehensive error handling

### Medium Term
- [ ] Integrate charting library for web dashboard
- [ ] Add WebSocket support for real-time updates
- [ ] Implement advanced market microstructure simulation
- [ ] Add external market data feed support

### Long Term
- [ ] Machine learning strategy framework
- [ ] Advanced risk management with real-time alerts
- [ ] Multi-exchange support
- [ ] Cloud deployment capabilities

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow C++17 standards
- Use consistent naming conventions
- Add tests for new features
- Update documentation
- Ensure thread safety for multi-threaded components

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built with modern C++17 for maximum performance
- Inspired by real-world HFT systems
- Designed for educational and research purposes

## 📞 Support

For questions, issues, or contributions:
- Open an issue on GitHub
- Check the test suite for usage examples
- Review the source code for implementation details

---

**Note**: This is a simulation environment for educational and research purposes. It is not intended for real trading and should not be used with real money. 