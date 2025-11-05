//
// Created by jtwears on 9/15/25.
//

#include <memory>
#include <thread>

#include "binancehistoricaldatafetcher/HistoricalDataProcessor.h"
#include "binancehistoricaldatafetcher/file_downloader.h"
#include "common/io/questdb_writer.h"
#include "common/sync/producer_consumer.h"

namespace binance::processor {

    HistoricalDataProcessor::HistoricalDataProcessor(
            const std::shared_ptr<Context> &context,
            std::unique_ptr<writer::QuestDBWriter> writer,
            std::unique_ptr<downloader::FileDownloader> downloader,
            std::unique_ptr<Settings> app_settings) :
    context_(context),
    writer_(std::move(writer)),
    downloader_(std::move(downloader)),
    settings_(std::move(app_settings)) {}

    void HistoricalDataProcessor::process() {
        std::thread producer([this]() {
            downloader_->download(settings_->symbols, settings_->startDate, settings_->endDate);
        });

        std::thread consumer([this]() {
           writer_->write();
        });

        producer.join();
        consumer.join();
    }
}
