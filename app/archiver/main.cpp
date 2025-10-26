//
// Created by jtwears on 9/27/25.
//

#include <string>
#include <thread>
#include <memory>
#include <CLI11.hpp>
#include <iostream>
#include <concurrentqueue/concurrentqueue.h>
#include "binancehistoricaldatafetcher/binance_futures_orderbook.h"
#include "binancehistoricaldatafetcher/binance_futures_book_builder.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"
#include "binancehistoricaldatafetcher/orderbook_archiver.h"
#include "binancehistoricaldatafetcher/questdb_writer.h"

constexpr auto ARCHIVER_VERSION = "0.1.0";
constexpr auto APP_NAME = "Binance Futures Market Data Archiver";
constexpr auto DEFAULT_QUESTDB_URL = "http::addr=localhost:9000";
constexpr auto DEFAULT_WEBSOCKET_URL = "wss://fstream.binance.com/stream";
constexpr auto DEFAULT_SYMBOLS = "btcusdt";
constexpr size_t DEFAULT_DEPTH = 20;
constexpr auto DEFAULT_WEBSOCKET_METHOD = "SUBSCRIBE";
constexpr auto DEFAULT_SNAPSHOT_PARAMS_FORMAT = "@depth20@100ms";
constexpr auto BTCUSDT_TICK_SIZE = 2;
constexpr auto BTCUSDT_STEP_SIZE = 3;

std::vector<std::string> get_symbols(std::string syms) {
    std::vector<std::string> symbols;
    size_t pos = 0;
    while ((pos = syms.find(',')) != std::string::npos) {
        std::string token = syms.substr(0, pos);
        symbols.push_back(token);
        syms.erase(0, pos + 1);
    }
    symbols.push_back(syms);
    return symbols;
}

models::BinanceFuturesOnOpenSocketMessage build_on_open_message(const std::vector<std::string>& symbols) {
    models::BinanceFuturesOnOpenSocketMessage msg;
    msg.method = DEFAULT_WEBSOCKET_METHOD;
    for (const auto& symbol : symbols) {
        msg.params.push_back(symbol + DEFAULT_SNAPSHOT_PARAMS_FORMAT);
    }
    msg.id = 1;
    return msg;
}

struct config {
    std::string websocket_url;
    std::vector<std::string> symbols;
    size_t depth;
    std::string questdb_url;
    models::BinanceFuturesOnOpenSocketMessage socket_open_msg;
};

config parse_command_line(int argc, char** argv) {
    CLI::App app{APP_NAME};
    std::string websocket_url = DEFAULT_WEBSOCKET_URL;
    app.add_option("--websocket_url", websocket_url, "WebSocket URL for Binance Futures")->default_val(DEFAULT_WEBSOCKET_URL);
    std::string symbols_str = DEFAULT_SYMBOLS;
    app.add_option("--symbols", symbols_str, "Comma-separated list of trading pairs")->default_val(DEFAULT_SYMBOLS);
    size_t depth = DEFAULT_DEPTH;
    app.add_option("--depth", depth, "Order book depth to maintain")->default_val(std::to_string(DEFAULT_DEPTH));
    std::string questdb_url = DEFAULT_QUESTDB_URL;
    app.add_option("--questdb_url", questdb_url, "QuestDB HTTP URL")->default_val(DEFAULT_QUESTDB_URL);
    app.parse(argc, argv);
    // add options here as needed
    config cfg;
    cfg.websocket_url = websocket_url;
    cfg.symbols = get_symbols(symbols_str);
    cfg.depth = depth;
    cfg.questdb_url = questdb_url;
    cfg.socket_open_msg = build_on_open_message(cfg.symbols);
    return cfg;
}

auto build_exchange_info_map() {
    std::unordered_map<std::string, models::ExchangeInfo> exchange_info;
    models::ExchangeInfo info{};
    info.tick_size = BTCUSDT_TICK_SIZE;
    info.step_size = BTCUSDT_STEP_SIZE;
    exchange_info["btcusdt"] = info;
    return exchange_info;
}

int build_and_start_snapshot_archiver(const config& cfg) {
    return 0;
}

int archiver_factory(const config& cfg) {
    return 0;
}

int main(const int argc, char** argv) {
    auto [websocket_url, symbols, depth, questdb_url, socket_open_msg] = parse_command_line(argc, argv);
    auto exchange_info = std::make_shared<std::unordered_map<std::string, models::ExchangeInfo>>(build_exchange_info_map());
    auto multi_symbol_orderbook = std::make_shared<models::BinanceFuturesOrderbook>(
        symbols,
        settings::Product::FUTURES,
        exchange_info,
        depth
    );
    auto data_events_queue = moodycamel::ConcurrentQueue<models::DataEvent>();
    auto socket_client = std::make_unique<downloader::BinanceFuturesOrderbookSnapshotsSocketClient>(
        websocket_url,
        socket_open_msg,
        multi_symbol_orderbook->get_queues(),
        exchange_info
    );
    auto book_builder = std::make_unique<processor::BinanceFuturesBookBuilder>(
        multi_symbol_orderbook,
        std::move(socket_client),
        data_events_queue,
        depth
    );
    auto questdb_writer = std::make_unique<writer::QuestDBWriter>(
        data_events_queue,
        questdb_url,
        std::make_shared<processor::Context>(), // Not ideal, but close will cancel the worker loop
        exchange_info,
        5,
        1000,
        settings::SNAPSHOT
    );
    const auto archiver = std::make_unique<processor::OrderbookArchiver>(
        std::move(book_builder),
        std::move(socket_client),
        std::move(questdb_writer)
    );
    archiver->start();

    if (!archiver->runningFlag()) {
        std::cerr << "ERROR::Failed to start archiver\n";
        return EXIT_FAILURE;
    }

    while (archiver->runningFlag()) {
        std::cout << "INFO::Running..." << "\n";
        std::this_thread::sleep_for(seconds(1));
    }
    if (const auto res = archiver->stop(); res != 0) {
        std::cerr << "ERROR::Failed to stop archiver cleanly" << std::endl;
        return res;
    }
    std::cout << "INFO::Archiver stopped successfully. Exiting." << std::endl;
    return EXIT_SUCCESS;
}