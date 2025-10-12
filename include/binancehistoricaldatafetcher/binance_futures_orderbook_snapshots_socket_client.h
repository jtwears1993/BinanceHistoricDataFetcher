//
// Created by jtwears on 10/5/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H


#include <nlohmann/json.hpp>
#include <concurrentqueue/concurrentqueue.h>

#include "binance_futures_socket_client.h"
#include "binance_market_data_models.h"

namespace downloader {
    class BinanceFuturesOrderbookSnapshotsSocketClient final : public BinanceFuturesSocketClient<models::BinanceFuturesSocketDepthSnapshot> {

    public:
        explicit BinanceFuturesOrderbookSnapshotsSocketClient(const std::string &uri,
            const models::BinanceFuturesOnOpenSocketMessage &open_msg,
            const std::unordered_map<std::string, moodycamel::ConcurrentQueue<models::BinanceFuturesSocketDepthSnapshot>*> &events_queue) :
            BinanceFuturesSocketClient(uri, open_msg, events_queue) {}

        void on_message(websocketpp::connection_hdl, client::message_ptr msg) override;
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_SNAPSHOTS_SOCKET_CLIENT_H