//
// Created by jtwears on 9/15/25.
//

#pragma once

#include <memory>

#include "binancehistoricaldatafetcher/settings.h"
#include "common/sync/producer_consumer.h"

using namespace common::sync::producer_consumer;

namespace downloader { class FileDownloader; }
namespace writer { class QuestDBWriter; }

namespace binance::processor {

    class HistoricalDataProcessor {
        std::shared_ptr<Context> context_;
        std::unique_ptr<writer::QuestDBWriter> writer_;
        std::unique_ptr<downloader::FileDownloader> downloader_;
        std::unique_ptr<settings::Settings> settings_;

    public:
        explicit HistoricalDataProcessor(const std::shared_ptr<Context> &context,
            std::unique_ptr<writer::QuestDBWriter> writer,
            std::unique_ptr<downloader::FileDownloader> downloader,
            std::unique_ptr<settings::Settings> app_settings);
        ~HistoricalDataProcessor() = default;

        void process();
    };
}


