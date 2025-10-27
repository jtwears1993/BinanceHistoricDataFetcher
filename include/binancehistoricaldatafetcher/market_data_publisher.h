//
// Created by jtwears on 10/26/25.
//
#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <concurrentqueue/concurrentqueue.h>

#include "binance_futures_book_builder.h"
#include "binance_market_data_models.h"
#include "common/network/socket/multicast_server.h"


namespace processor {
    class MarketDataPublisher {
        static std::atomic_bool is_running_;
        std::unique_ptr<common::network::sockets::MulticastServer> updates_socket_;
        std::unique_ptr<BinanceFuturesBookBuilder> book_builder_;
        moodycamel::ConcurrentQueue<models::DataEvent>& data_event_queue_;
        std::thread server_thread_;
        std::atomic<size_t> sequence_id_ = 1;

    public:
        explicit  MarketDataPublisher(
            std::unique_ptr<common::network::sockets::MulticastServer> updates_socket,
            std::unique_ptr<BinanceFuturesBookBuilder> book_builder,
            moodycamel::ConcurrentQueue<models::DataEvent>& data_event_queue
        ) : updates_socket_(std::move(updates_socket)),
            book_builder_(std::move(book_builder)),
            data_event_queue_(data_event_queue) {};

        ~MarketDataPublisher() noexcept {
            if (is_running()) {
                stop();
            }
        };
        // Deleted default, copy & move constructors and assignment-operators.
        MarketDataPublisher(const MarketDataPublisher &) = delete;
        MarketDataPublisher(const MarketDataPublisher &&) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher &) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher &&) = delete;

        void start();

        void stop() noexcept;

        void run() noexcept;

        bool is_running() const {
            return is_running_.load();
        }

        static void handle_signals(int signum);
    };
}
