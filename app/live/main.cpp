//
// Created by jtwears on 9/27/25.
//


#include <string>
#include <thread>
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <concurrentqueue/concurrentqueue.h>
#include "binancehistoricaldatafetcher/binance_futures_orderbook.h"
#include "binancehistoricaldatafetcher/binance_futures_book_builder.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"
#include "binancehistoricaldatafetcher/market_data_publisher.h"
#include "binancehistoricaldatafetcher/settings.h"
#include "common/network/socket/multicast_server.h"
#include "common/network/socket/utils.h"


constexpr auto LIVE_APP_MCAST_VERSION = "0.1.0";
constexpr auto APP_NAME = "Binance Futures Market Data Publisher";
constexpr auto DEFAULT_WEBSOCKET_URL = "wss://fstream.binance.com/stream";
constexpr auto DEFAULT_SYMBOLS = "btcusdt";
constexpr size_t DEFAULT_DEPTH = 20;
constexpr auto DEFAULT_WEBSOCKET_METHOD = "SUBSCRIBE";
constexpr auto DEFAULT_SNAPSHOT_PARAMS_FORMAT = "@depth20@100ms";
constexpr auto BTCUSDT_TICK_SIZE = 2;
constexpr auto BTCUSDT_STEP_SIZE = 3;
constexpr auto mkt_pub_iface = "lo";
constexpr auto snap_pub_ip = "233.252.14.1";
constexpr auto snap_pub_port = 20000;
constexpr auto DEFAULT_PRODUCT_CLASS = settings::Product::FUTURES;
constexpr auto DEFAULT_DATA_TYPE = settings::DataType::SNAPSHOT;
constexpr auto DATA_EVENT_QUEUE_SIZE = 1000;


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

auto build_exchange_info_map() {
   std::unordered_map<std::string, models::ExchangeInfo> exchange_info;
   models::ExchangeInfo info{};
   info.tick_size = BTCUSDT_TICK_SIZE;
   info.step_size = BTCUSDT_STEP_SIZE;
   exchange_info["btcusdt"] = info;
   return exchange_info;
}

struct orderbook_setings {
   size_t depth{DEFAULT_DEPTH};
};

struct config {
   settings::Product product_class{DEFAULT_PRODUCT_CLASS};
   settings::DataType data_type{DEFAULT_DATA_TYPE};
   orderbook_setings orderbook_settings{
      DEFAULT_DEPTH
   };
   std::string websocket_url{DEFAULT_WEBSOCKET_URL};
   std::vector<std::string> symbols{get_symbols(DEFAULT_SYMBOLS)};
   models::BinanceFuturesOnOpenSocketMessage socket_open_msg{build_on_open_message(symbols)};
   common::network::sockets::SocketConfig socket_config{
      snap_pub_ip,
      mkt_pub_iface,
      snap_pub_port,
      common::network::sockets::SocketType::UDP,
      false,
      false
   };
};

auto parse_config(int argc, char* argv[]) {
   return config{};
}

auto build_market_data_publisher(const config& cfg, moodycamel::ConcurrentQueue<models::DataEvent> &data_events_buffer) {
   auto exchange_info = std::make_shared<std::unordered_map<std::string, models::ExchangeInfo>>(build_exchange_info_map());
   auto multi_symbol_orderbook = std::make_shared<models::BinanceFuturesOrderbook>(
      cfg.symbols,
      DEFAULT_PRODUCT_CLASS,
      exchange_info,
      cfg.orderbook_settings.depth
   );
   auto book_builder = std::make_unique<processor::BinanceFuturesBookBuilder>(
      multi_symbol_orderbook,
      std::make_unique<downloader::BinanceFuturesOrderbookSnapshotsSocketClient>(
         cfg.websocket_url,
         cfg.socket_open_msg,
         multi_symbol_orderbook->get_queues(),
         exchange_info
      ),
      data_events_buffer,
      cfg.orderbook_settings.depth
   );
   auto updates_socket = std::make_unique<common::network::sockets::MulticastServer>(cfg.socket_config);
   return std::make_unique<processor::MarketDataPublisher>(
      std::move(updates_socket),
      std::move(book_builder),
      data_events_buffer
   );
}

int main(const int argc, char **argv) {
   const auto cfg = parse_config(argc, argv);
   auto data_events_buffer = moodycamel::ConcurrentQueue<models::DataEvent>(DATA_EVENT_QUEUE_SIZE);
   const auto market_data_publisher = build_market_data_publisher(cfg, data_events_buffer);
   if (!market_data_publisher) {
      std::cerr << "ERROR::Failed to build MarketDataPublisher\n";
      return EXIT_FAILURE;
   }
   market_data_publisher->start();
   while (market_data_publisher->is_running()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }
   market_data_publisher->stop();
   return EXIT_SUCCESS;
}