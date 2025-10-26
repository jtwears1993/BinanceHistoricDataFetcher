//
// Created by jtwears on 10/5/25.
//
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"

namespace downloader {

    using json = nlohmann::json;

    void BinanceFuturesOrderbookSnapshotsSocketClient::on_message(websocketpp::connection_hdl, const client::message_ptr msg) {
        auto event = msg->get_payload();
        const auto snapshot = json::parse(event);
        [[unlikely]] if (!snapshot.contains("data")) {
            // No event type; ignore
            std::cout << "INFO::BinanceFuturesOrderbookSnapshotsSocketClient::on_message Ignoring message with no event type: " << snapshot.dump() << std::endl;
            return;
        }

        [[unlikely]] if (snapshot.contains("data") && snapshot["data"]["e"] != "depthUpdate") {
            // Not a depth update message; ignore
            std::cout << "INFO::BinanceFuturesOrderbookSnapshotsSocketClient::on_message Ignoring non-depthUpdate message: " << snapshot.dump() << std::endl;
            return;
        }
        std::cout << "INFO::BinanceFuturesOrderbookSnapshotsSocketClient::on_message Received depth update message: " << snapshot.dump() << std::endl;
        models::BinanceFuturesSocketDepthSnapshot snapshot_data;
        from_json(snapshot["data"], snapshot_data);
        if (const auto symbol_buffer = event_queues_.find(snapshot_data.symbol); symbol_buffer != event_queues_.end()) {
            symbol_buffer->second->enqueue(snapshot_data);
        } else {
            std::cout << "No queue found for symbol: " << snapshot_data.symbol << std::endl;
            throw std::runtime_error("No queue found for symbol");
        }
    }

    void BinanceFuturesOrderbookSnapshotsSocketClient::from_json(const nlohmann::json &j, models::BinanceFuturesSocketDepthSnapshot &snapshot) const {
        auto order_book_symbol = j["s"].get<std::string>();
        std::ranges::transform(order_book_symbol, order_book_symbol.begin(),::tolower);
        snapshot.symbol = order_book_symbol;
        j.at("e").get_to(snapshot.event_type);
        j.at("E").get_to(snapshot.event_time);
        j.at("T").get_to(snapshot.transaction_time);
        j.at("U").get_to(snapshot.first_update_id);
        j.at("u").get_to(snapshot.final_update_id);
        j.at("pu").get_to(snapshot.previous_final_update_id);
        std::vector<models::PriceLevel> bids;
        std::vector<models::PriceLevel> asks;
        for (const auto &bid : j["b"]) {
            models::PriceLevel price_level{};
            parse_price_levels(bid, price_level, snapshot.symbol);
            bids.push_back(price_level);
        }
        for (const auto &ask : j["a"]) {
            models::PriceLevel price_level{};
            parse_price_levels(ask, price_level, snapshot.symbol);
            asks.push_back(price_level);
        }
        snapshot.bids = bids;
        snapshot.asks = asks;
    }

    void BinanceFuturesOrderbookSnapshotsSocketClient::parse_price_levels(const nlohmann::json &j, models::PriceLevel &price_level, const std::string &symbol) const {
        if (j.is_array() && j.size() == 2) {
            auto [tick_size, step_size] = exchange_info_->at(symbol);
            price_level.price = common::rounding::FixedPoint::from_string(j.at(0).get<std::string>(), tick_size);
            price_level.quantity = common::rounding::FixedPoint::from_string(j.at(1).get<std::string>(), step_size);
        } else {
            throw std::runtime_error("Invalid PriceLevel JSON format");
        }
    }

};