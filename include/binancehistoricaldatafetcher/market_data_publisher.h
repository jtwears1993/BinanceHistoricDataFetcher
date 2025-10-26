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
        std::atomic_bool is_running_ = false;
        std::unique_ptr<common::network::sockets::MulticastServer> updates_socket_;
        std::unique_ptr<BinanceFuturesBookBuilder> book_builder_;
        moodycamel::ConcurrentQueue<models::DataEvent>& data_event_queue_;

    public:
        explicit  MarketDataPublisher(
            std::unique_ptr<common::network::sockets::MulticastServer> updates_socket,
            std::unique_ptr<BinanceFuturesBookBuilder> book_builder,
            moodycamel::ConcurrentQueue<models::DataEvent>& data_event_queue
        );
        ~MarketDataPublisher() = default;
        // Deleted default, copy & move constructors and assignment-operators.
        MarketDataPublisher(const MarketDataPublisher &) = delete;
        MarketDataPublisher(const MarketDataPublisher &&) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher &) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher &&) = delete;

        void start() {

        }

        void stop() {
            
        }

        void run();
    };
}
