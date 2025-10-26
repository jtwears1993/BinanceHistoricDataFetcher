//
// Created by jtwears on 9/29/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H

#include <string>
#include <memory>
#include <unordered_map>
#include <concurrentqueue/concurrentqueue.h>

#include "binancehistoricaldatafetcher/binance_orderbook.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"
#include "common/rounding/fixed_point.h"

namespace models {

    constexpr auto PROD_BINANCE_FUTURES_REST_URL = "https://fapi.binance.com/fapi/v1/depth";

    struct Context {
        unsigned long long last_update_id; // Last update ID in snapshot
        unsigned long long previous_u; // First update ID in event
        bool is_initialized = false;
        std::shared_ptr<moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot>> price_level_queue;
    };

    class BinanceFuturesOrderbook final : public BinanceOrderbook<BinanceFuturesSocketDepthSnapshot> {
        std::unordered_map<std::string, Context> context_;
        size_t depth_;

    public:

        explicit BinanceFuturesOrderbook(const std::vector<std::string> &symbols,
            const settings::Product product,
            const std::shared_ptr<std::unordered_map<std::string, ExchangeInfo>> &exchange_info,
            const size_t depth) :
            BinanceOrderbook(symbols, product, exchange_info), depth_(depth) {

            for (const auto &symbol : symbols) {
                auto ctx = Context{};
                ctx.price_level_queue = std::make_shared<moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot>>();
                context_.emplace(symbol, ctx);
            }
        }

        std::unordered_map<std::string, std::shared_ptr<moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot>>> get_queues() const {
            std::unordered_map<std::string, std::shared_ptr<moodycamel::ConcurrentQueue<BinanceFuturesSocketDepthSnapshot>>> queues;
            for (const auto & [symbol, ctx] : context_) {
                queues.emplace(symbol, ctx.price_level_queue);
            }
            return queues;
        }

        void init(const std::vector<std::string>& symbols) override;

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
            symbol_context->second.price_level_queue->enqueue(snapshot);
            return true;
        }

        std::optional<std::vector<BinanceFuturesSocketDepthSnapshot>> deque_update(const std::string& symbol) {
            const auto it = context_.find(symbol);
            [[unlikely]]  if (it == context_.end()) {
                return std::nullopt;
            }
            const auto& queue = it->second.price_level_queue;
            const size_t size = queue->size_approx();
            if (size == 0) {
                return std::nullopt;
            }
            std::vector<BinanceFuturesSocketDepthSnapshot> snapshots(size);
            const size_t actual = queue->try_dequeue_bulk(snapshots.data(), size);
            if (actual == 0) {
                return std::nullopt;
            }
            snapshots.resize(actual);
            return snapshots;
        }

        void init_order_book(const std::string& symbol);

        std::vector<std::string> get_symbols() const {
            std::vector<std::string> symbols;
            for (const auto &symbol: context_ | std::views::keys) {
                symbols.push_back(symbol);
            }
            return symbols;
        }
    private:
        void apply_update(const std::string &symbol, const std::vector<PriceLevel> &price_level, bool is_bid);

        std::optional<BinanceFuturesOrderbookSnapshot> fetch_snapshot(const std::string &symbol) const;
    };

}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_ORDERBOOK_H