//
// Created by jtwears on 10/12/25.
//

#include <csignal>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include "binancehistoricaldatafetcher/orderbook_archiver.h"

namespace binance::processor {

    // define the static member
    std::atomic<bool> OrderbookArchiver::is_running_{false};

    OrderbookArchiver::~OrderbookArchiver() noexcept {
        // safety-net: ensure we stop on destruction
        try {
            stop();
        } catch (...) {
            // destructors must not let exceptions escape
        }
    }

    void OrderbookArchiver::start() {
        // idempotent: if already running, return
        if (is_running_.exchange(true)) {
            return; // already running
        }

        // register signal handlers to allow Ctrl-C to stop the archiver (safe to call repeatedly)
        std::signal(SIGINT, handle_signals);
        std::signal(SIGQUIT, handle_signals);
        std::signal(SIGTERM, handle_signals);
        std::signal(SIGTERM, handle_signals);
        try {

            if (book_builder_) {
                book_builder_->start();
            }

            if (quest_db_writer_) {
                // run writer in its own thread; write() should block until stopped/closed
                writer_thread_ = std::thread([this]() {
                    try {
                        quest_db_writer_->write();
                    } catch (...) {
                        // swallow; writer should handle its own errors
                        std::cerr << "FATAL::OrderbookArchiver::start Error starting questdb writer" << std::endl;
                        throw;
                    }
                });
            }
        } catch (...) {
            // If starting failed, ensure we clear running flag and attempt a clean stop
            std::cerr << "FATAL::failed to start orderbook archiver thread \n";
            is_running_.store(false);
            try {
                stop();
            } catch (...) {
                std::cerr << "FATAL::exception occurred during orderbook archiver stop after failed start" << std::endl;
            }
            auto exception = std::current_exception();
            throw;
        }
    }

    int OrderbookArchiver::stop() noexcept {
        // idempotent: if already false, return
        if (!is_running_.exchange(false)) {
            std::cout << "INFO::OrderbookArchiver already stopped, returning from stop()\n";
            return EXIT_SUCCESS; // already stopped
        }

        std::vector<int> error_codes;
        try {
            if (book_builder_) {
                book_builder_->stop();
            }
        } catch (...) {
            std::cerr << "FATAL::failed to stop BookBuilder \n";
            error_codes.emplace_back(-1);
        }

        try {
            if (quest_db_writer_) {
                quest_db_writer_->close();
            }
        } catch (...) {
            std::cerr << "FATAL::failed to stop quest DB writer \n";
            error_codes.emplace_back(-1);
        }

        // join writer thread if running
        try {
            if (writer_thread_.joinable()) {
                writer_thread_.join();
            }
        } catch (...) {
            std::cerr << "FATAL::failed to join writer_thread \n";
            error_codes.emplace_back(-1);
        }

        if (!error_codes.empty()) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }


    void OrderbookArchiver::handle_signals(const int signum) {
        // signal handlers must be async-signal-safe; only set flag here
        is_running_.store(false);
    }

    std::atomic<bool>& OrderbookArchiver::runningFlag() {
        return is_running_;
    }

} // namespace processor
