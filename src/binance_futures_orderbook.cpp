//
// Created by jtwears on 9/29/25.
//

#include <string>
#include <vector>
#include <thread>
#include <iostream>

#include "binancehistoricaldatafetcher/binance_orderbook.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/parameters.h"

namespace models {

    void BinanceFuturesOrderbook::init(const std::vector<std::string>& symbols) {
        std::vector<std::thread> threads;
        for (const auto& snapshot : symbols) {
            threads.emplace_back([this, &snapshot]() {
                this->init_order_book(snapshot);
            });
        }
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void BinanceFuturesOrderbook::init_order_book(const std::string& symbol) {
        const auto snapshot_ = fetch_snapshot(symbol);
        [[unlikely]] if (!snapshot_.has_value()) {
            throw std::runtime_error("Failed to get snapshot for symbol: " + symbol);
        }
        auto snapshot = snapshot_.value();
        const auto symbol_context = context_.find(snapshot.symbol);
        [[unlikely]] if (symbol_context == context_.end()) {
            throw std::invalid_argument("Symbol not found in orderbook context");
        }

        while (!symbol_context->second.is_initialized) {
           auto socket_events = deque_update(snapshot.symbol);
            if (!socket_events.has_value()) {
                continue;
            }

            // Drop events with little u less that snapshot lat update id
            std::vector<BinanceFuturesSocketDepthSnapshot> valid_events;
            for (const auto &event : socket_events.value()) {
                if (event.final_update_id < snapshot.lastUpdate_id) {
                    continue;
                }
                valid_events.push_back(event);
            }

            // first processed event is U <= last_update_id AND u >= last_update_id
            // break after this and set symbol context to initialised true
            for (const auto &event : valid_events) {
                if (event.first_update_id <= snapshot.lastUpdate_id
                    && event.final_update_id >= snapshot.lastUpdate_id) {
                    apply_update(event.symbol, event.bids, true);
                    apply_update(event.symbol, event.asks, false);
                    symbol_context->second.last_update_id = event.final_update_id;
                    symbol_context->second.previous_u = event.final_update_id;
                    symbol_context->second.is_initialized = true;
                    break;
                }
                // hit this snapshot is stale
                // fetch new snapshot
                snapshot = fetch_snapshot(symbol).value();
            }
        }
    }


    /*
     * Error Codes:
     *  0 - Success
     * -1 - Out of sync
     */
    int BinanceFuturesOrderbook::process_update(const BinanceFuturesSocketDepthSnapshot& snapshot) {
        const auto symbol_context = context_.find(snapshot.symbol);
        [[unlikely]] if (symbol_context == context_.end()) {
            throw std::invalid_argument("Symbol not found in orderbook context");
        }

        if (snapshot.previous_final_update_id != symbol_context->second.last_update_id) {
            symbol_context->second.is_initialized = false;
            return -1;
        }

        apply_update(snapshot.symbol, snapshot.bids, true);
        apply_update(snapshot.symbol, snapshot.asks, false);
        symbol_context->second.last_update_id = snapshot.final_update_id;
        symbol_context->second.previous_u = snapshot.final_update_id;
        return 0;
    }

    void BinanceFuturesOrderbook::apply_update(const std::string &symbol, const std::vector<PriceLevel> &price_level, const bool is_bid) {
        for (const auto &level : price_level) {
            if (level.quantity > 0) {
                multi_symbol_orderbook_.update_price_level(symbol, level, is_bid);
            } else {
                multi_symbol_orderbook_.remove_price_level(symbol, level, is_bid);
            }
        }
    }

    std::optional<BinanceFuturesOrderbookSnapshot> BinanceFuturesOrderbook::fetch_snapshot(const std::string &symbol) const {
        auto depth = std::to_string(depth_);
        const auto response = cpr::Get(cpr::Url{PROD_BINANCE_FUTURES_REST_URL},
                                       cpr::Parameters{{"symbol", symbol},
                                                       {"limit", depth}});

        if (response.status_code != 200) {
            return std::nullopt;
        }
        BinanceFuturesOrderbookSnapshot snapshot;
        auto [tick_size, step_size] = exchange_info_->at(symbol);
        models::from_json(nlohmann::json::parse(response.text), snapshot, tick_size, step_size);
        snapshot.symbol = symbol;
        return snapshot;
    }
}

