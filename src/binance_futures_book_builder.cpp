//
// Created by jtwears on 10/5/25.
//

#include <string>
#include <vector>
#include <thread>
#include <cpr/cpr.h>


#include "binancehistoricaldatafetcher/binance_futures_book_builder.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"

namespace processor {

    using json = nlohmann::json;

    void BinanceFuturesBookBuilder::start() {
        auto res = socket_client_->start();
        if (res != EXIT_SUCCESS) {
            std::cerr << "ERROR::BinanceFuturesBookBuilder:: Error starting socket client: " << res << std::endl;
            throw std::runtime_error("Failed to start BinanceFuturesBookBuilder socket client");
        }
        order_books_->init(symbols_);
        // start threads to process updates
        is_running_ = true;
        for (const auto &symbol : symbols_) {
            // start thread and add to vector
            builder_threads_.emplace_back(&BinanceFuturesBookBuilder::build_book, this, symbol);
        }
    }

    void BinanceFuturesBookBuilder::stop() {
        socket_client_->stop();
        is_running_ = false;
        for (auto &thread : builder_threads_) {
            thread.join();
        }
    }

    // get a snapshot for each symbol
    // publish a bulk message to the queue
    // DataEvent
    void BinanceFuturesBookBuilder::get_snapshots() const {
        std::vector<models::DataEvent> snapshots;
        for (const auto &symbol : symbols_) {
            models::DataEvent event;
            auto snapshot = order_books_->get_snapshot(symbol, depth_);
            event.orderbook_snapshot = snapshot;
            snapshots.emplace_back(event);
        }
        event_queue_.enqueue_bulk(snapshots.data(), snapshots.size());
    }

    void BinanceFuturesBookBuilder::build_book(const std::string &symbol) const {
        while (is_running_) {
            auto updates = order_books_->deque_update(symbol);
            if (!updates.has_value()) {
                continue;
            }
            for (const auto &update : updates.value()) {
                auto res = order_books_->process_update(update);
                if (res == -1) {
                    // get fresh snapshot and re-init
                    std::cout << "WARN::BinanceFuturesBookBuilder::build_book Re-initializing order book for symbol: " << symbol << "\n";
                    order_books_->init_order_book(symbol);
                    break;
                }
                // publish update event
                models::DataEvent event;
                event.orderbook_snapshot = order_books_->get_snapshot(symbol, depth_);
                event_queue_.enqueue(event);
                std::cout << "INFO::BinanceFuturesBookBuilder::build_book Published update for symbol: " << symbol << "\n";
            }
        }
    }
}