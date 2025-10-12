//
// Created by jtwears on 9/29/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H

#include <string>
#include <unordered_map>
#include <concurrentqueue/concurrentqueue.h>

#include "binancehistoricaldatafetcher/binance_orderbook.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"

namespace models {

    struct Context {
        unsigned long long last_update_id; // Last update ID in snapshot
        unsigned long long previous_u; // First update ID in event
        bool is_initialized = false;
        moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot> price_level_queue;
    };

    class BinanceFuturesOrderbook final : public BinanceOrderbook<BinanceFuturesSocketDepthSnapshot, BinanceFuturesOrderbookSnapshot> {
        std::unordered_map<std::string, Context> context_;
        size_t depth_;

    public:

        explicit BinanceFuturesOrderbook(const std::vector<std::string> &symbols, const settings::Product product, const size_t depth) :
            BinanceOrderbook(symbols, product), depth_(depth) {}

        void init(const std::vector<BinanceFuturesOrderbookSnapshot>& snapshots) override;

        int process_update(const BinanceFuturesSocketDepthSnapshot& snapshot) override;

        bool is_initialized(const std::string& symbol) const override {
            const auto symbol_context = context_.find(symbol);
            [[unlikely]] if (symbol_context == context_.end()) {
                throw std::runtime_error("Symbol not found");
            }
            return symbol_context->second.is_initialized;
        }

        bool enque_update(const BinanceFuturesSocketDepthSnapshot& snapshot) {
            const auto symbol_context = context_.find(snapshot.symbol);
            [[unlikely]] if (symbol_context == context_.end()) {
                return false;
            }
            symbol_context->second.price_level_queue.enqueue(snapshot);
            return true;
        }

        std::optional<std::vector<BinanceFuturesSocketDepthSnapshot>> deque_update(const std::string& symbol) {
            const auto it = context_.find(symbol);
            [[unlikely]]  if (it == context_.end()) {
                return std::nullopt;
            }
            auto& queue = it->second.price_level_queue;
            const size_t size = queue.size_approx();
            if (size == 0) {
                return std::nullopt;
            }
            std::vector<BinanceFuturesSocketDepthSnapshot> snapshots(size);
            const size_t actual = queue.try_dequeue_bulk(snapshots.data(), size);
            if (actual == 0) {
                return std::nullopt;
            }
            snapshots.resize(actual);
            return snapshots;
        }

        void init_order_book(const BinanceFuturesOrderbookSnapshot& snapshot);

        std::vector<std::string> get_symbols() const {
            std::vector<std::string> symbols;
            for (const auto &symbol: context_ | std::views::keys) {
                symbols.push_back(symbol);
            }
            return symbols;
        }
    private:
        void apply_update(const std::string &symbol, const std::vector<PriceLevel> &price_level, bool is_bid);
    };

}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H