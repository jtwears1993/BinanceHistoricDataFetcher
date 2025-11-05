//
// Created by jtwears on 10/12/25.
//

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include "binancehistoricaldatafetcher/binance_futures_book_builder.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"
#include "common/io/questdb_writer.h"

namespace binance::processor {
    class OrderbookArchiver {
        // make the running flag static so a static signal handler can modify it
        static std::atomic<bool> is_running_;
        std::unique_ptr<BinanceFuturesBookBuilder> book_builder_;
        std::unique_ptr<writer::QuestDBWriter> quest_db_writer_;
        std::thread writer_thread_;
    public:
        OrderbookArchiver(
            std::unique_ptr<BinanceFuturesBookBuilder> book_builder,
            std::unique_ptr<writer::QuestDBWriter> quest_db_writer
        ) :
        book_builder_(std::move(book_builder)),
        quest_db_writer_(std::move(quest_db_writer)) {}
        ~OrderbookArchiver() noexcept;

        // signal handler must be static to be usable with std::signal
        static void handle_signals(int signum);

        // allow external code to observe the running flag (useful for main loop)
        static std::atomic<bool>& runningFlag();

        void start();
        int stop() noexcept;
    };
}
