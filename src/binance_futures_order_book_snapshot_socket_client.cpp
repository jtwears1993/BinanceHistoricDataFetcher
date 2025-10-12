//
// Created by jtwears on 10/5/25.
//

#include <nlohmann/json.hpp>

#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"

namespace downloader {

    using json = nlohmann::json;

    void BinanceFuturesOrderbookSnapshotsSocketClient::on_message(websocketpp::connection_hdl, const client::message_ptr msg) {
        auto event = msg->get_payload();
        const auto snapshot = json::parse(event);
        models::BinanceFuturesSocketDepthSnapshot snapshot_data;
        models::from_json(snapshot, snapshot_data);

        if (const auto symbol_buffer = event_queues_.find(snapshot_data.symbol); symbol_buffer != event_queues_.end()) {
            symbol_buffer->second->enqueue(snapshot_data);
        } else {
            std::cout << "No queue found for symbol: " << snapshot_data.symbol << std::endl;
        }
    }
};