//
// Created by jtwears on 10/5/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_BOOK_BUILDER_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_BOOK_BUILDER_H

#include <atomic>
#include <string>
#include <memory>
#include <optional>
#include <concurrentqueue/concurrentqueue.h>

#include "binance_futures_orderbook.h"
#include "binance_futures_orderbook_snapshots_socket_client.h"

namespace processor {

    constexpr auto PROD_BINANCE_FUTURES_REST_URL = "https://fapi.binance.com/fapi/v1/depth";

    class BinanceFuturesBookBuilder {
        std::shared_ptr<models::BinanceFuturesOrderbook> order_books_;
        std::unique_ptr<downloader::BinanceFuturesOrderbookSnapshotsSocketClient> socket_client_;
        std::atomic<bool> is_running_;
        std::vector<std::string> symbols_;
        std::vector<std::thread> builder_threads_;
        const size_t depth_;
        moodycamel::ConcurrentQueue<models::DataEvent>& event_queue_;

    public:
        BinanceFuturesBookBuilder(
            const std::shared_ptr<models::BinanceFuturesOrderbook> &order_books,
            std::unique_ptr<downloader::BinanceFuturesOrderbookSnapshotsSocketClient> socket_client,
            moodycamel::ConcurrentQueue<models::DataEvent>& event_queue,
            const size_t depth = 20) :
        order_books_(order_books),
        socket_client_(std::move(socket_client)),
        is_running_(false),
        depth_(depth),
        event_queue_(event_queue)
        {
            auto orderbook_symbols = order_books_->get_symbols();
            symbols_.insert(symbols_.end(), orderbook_symbols.begin(), orderbook_symbols.end());
        }
        ~BinanceFuturesBookBuilder() = default;
        void start();
        void stop();
    private:
        void get_snapshots() const;

        static std::optional<models::BinanceFuturesOrderbookSnapshot> get_snapshot(const std::string &symbol);
        void build_book(const std::string &symbol) const;
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_BOOK_BUILDER_H