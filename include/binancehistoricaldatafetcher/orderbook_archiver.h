//
// Created by jtwears on 10/12/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_ARCHIVER_H
#define BINANCEHISTORICDATAFETCHER_ARCHIVER_H

#include <atomic>
#include <csignal>
#include "binancehistoricaldatafetcher/settings.h"
#include "binancehistoricaldatafetcher/binance_futures_book_builder.h"
#include "binancehistoricaldatafetcher/binance_futures_orderbook_snapshots_socket_client.h"
#include "binancehistoricaldatafetcher/questdb_writer.h"


namespace processor {
    class Archiver {
        std::atomic<bool> is_running_;
        BinanceFuturesBookBuilder book_builder_;
        downloader::BinanceFuturesOrderbookSnapshotsSocketClient socket_client_;
        writer::QuestDBWriter quest_db_writer_;


    public:
        Archiver() : is_running_(false) {

        }
        ~Archiver() = default;
        void handle_signals();
        void start();
        void stop();
    };
}

#endif //BINANCEHISTORICDATAFETCHER_ARCHIVER_H