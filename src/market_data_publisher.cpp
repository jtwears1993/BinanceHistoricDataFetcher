//
// Created by jtwears on 10/27/25.
//

#include <thread>
#include <nlohmann/json.hpp>

#include "binancehistoricaldatafetcher/market_data_publisher.h"



using json = nlohmann::json;

namespace processor {

    std::atomic<bool> MarketDataPublisher::is_running_{false};

    void MarketDataPublisher::start() {
        // register signal handlers to allow Ctrl-C to stop the archiver (safe to call repeatedly)
        std::signal(SIGINT, handle_signals);
        std::signal(SIGQUIT, handle_signals);
        std::signal(SIGTERM, handle_signals);
        std::signal(SIGTERM, handle_signals);
        is_running_.store(true);
        server_thread_ = std::thread(&MarketDataPublisher::run, this);
        book_builder_->start();
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
        if (!updates_socket_->start()) {
            std::cerr << "FATAL::MarketDataPublisher::run failed to start MulticastServer\n";
            is_running_.store(false);
            return;
        }

        constexpr auto debug_msg = "C++ SENDER IS ALIVE";
        updates_socket_->send(debug_msg, strlen(debug_msg));
        updates_socket_->send_and_receive();
        std::cerr << "SENT DEBUG PACKET!\n";

        while (is_running_.load()) {
            if (models::DataEvent data_event; data_event_queue_.try_dequeue(data_event)) {
                try {
                    updates_socket_->send(&sequence_id_, sizeof(sequence_id_));
                    json j = data_event;
                    std::string j_str = j.dump();
                    uint32_t payload_size = j_str.length();
                    updates_socket_->send(j_str.data(), j_str.size());
                    updates_socket_->send(&payload_size, sizeof(payload_size));
                    updates_socket_->send_and_receive();
                    ++sequence_id_;
                    std::cout << "INFO::MarketDataPublisher::run published sequence_id: " << sequence_id_ - 1 << '\n';
                } catch (const std::exception& e) {
                    std::cerr << "ERROR::MarketDataPublisher::run exception: " << e.what() << '\n';
                }
            } else {
                std::this_thread::yield();
            }
        }
    }

    void MarketDataPublisher::handle_signals(int signum) {
        is_running_.store(false);
    }
}