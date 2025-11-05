//
// Created by jtwears on 10/5/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/websocketpp/client.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include <concurrentqueue/concurrentqueue.h>
#include <nlohmann/json.hpp>


#include "binance_futures_socket_client.h"
#include "binance_market_data_models.h"
#include "common/models/common_data_models.h"

using namespace binance::models;
using namespace common::models;

namespace downloader {
    class BinanceFuturesOrderbookSnapshotsSocketClient final : public BinanceFuturesSocketClient<BinanceFuturesSocketDepthSnapshot> {
        const std::shared_ptr<std::unordered_map<std::string, ExchangeInfo>> exchange_info_;

    public:
        explicit BinanceFuturesOrderbookSnapshotsSocketClient(const std::string &uri,
            const BinanceFuturesOnOpenSocketMessage &open_msg,
            const std::unordered_map<std::string,std::shared_ptr<moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot>>> &events_queue,
            const std::shared_ptr<std::unordered_map<std::string, ExchangeInfo>> &exchange_info) :
            BinanceFuturesSocketClient(uri, open_msg, events_queue),
            exchange_info_(exchange_info) {}

        void on_message(websocketpp::connection_hdl, client::message_ptr msg) override;

        void from_json(const nlohmann::json &j, BinanceFuturesSocketDepthSnapshot &snapshot) const;

        void parse_price_levels(const nlohmann::json &j, PriceLevel &price_level, const std::string &symbol) const;
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H