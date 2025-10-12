//
// Created by jtwears on 9/29/25.
//

#include <string>
#include <vector>
#include <thread>

#include "binancehistoricaldatafetcher/binance_orderbook.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"

namespace models {

    void BinanceFuturesOrderbook::init(const std::vector<BinanceFuturesOrderbookSnapshot>& snapshots) {
        std::vector<std::thread> threads;
        for (const auto& snapshot : snapshots) {
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

    void BinanceFuturesOrderbook::init_order_book(const BinanceFuturesOrderbookSnapshot &snapshot) {
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
}

