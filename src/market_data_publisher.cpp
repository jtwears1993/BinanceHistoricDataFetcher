//
// Created by jtwears on 10/27/25.
//

#include <thread>

#include "binancehistoricaldatafetcher/market_data_publisher.h"

namespace processor {

    void MarketDataPublisher::start() {
        // register signal handlers to allow Ctrl-C to stop the archiver (safe to call repeatedly)
        std::signal(SIGINT, handle_signals);
        std::signal(SIGQUIT, handle_signals);
        std::signal(SIGTERM, handle_signals);
        std::signal(SIGTERM, handle_signals);
        server_thread_ = std::thread(&MarketDataPublisher::run, this);
        book_builder_->start();
        is_running_.store(true);
    }

    void MarketDataPublisher::stop() noexcept {
        // idempotent: if already false, return
        if (!is_running_.exchange(false)) {
            return;
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        try {
            book_builder_->stop();
        } catch (...) {
            std::cerr << "FATAL::MarketDataPublisher::stop failed to cleanly stop BookBuilder \n";
        }
    }

    void MarketDataPublisher::run() noexcept {
        models::DataEvent data_event;
        while (is_running_.load()) {
            if (data_event_queue_.try_dequeue(data_event)) {
                updates_socket_->send(&sequence_id_, sizeof(size_t));
                updates_socket_->send(&data_event, sizeof(data_event));
                updates_socket_->increment_send_next_valid_index();
                updates_socket_->send_and_receive();
                ++sequence_id_;
            }
        }
    }

    void MarketDataPublisher::handle_signals(int signum) {
        is_running_.store(false);
    }
}