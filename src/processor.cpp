//
// Created by jtwears on 9/15/25.
//

#include <memory>
#include <thread>

#include "binancehistoricaldatafetcher/processor.h"
#include "binancehistoricaldatafetcher/settings.h"
#include "binancehistoricaldatafetcher/file_downloader.h"
#include "binancehistoricaldatafetcher/questdb_writer.h"

namespace processor {

    Processor::Processor(const std::shared_ptr<Context> &context,
            std::unique_ptr<writer::QuestDBWriter> writer,
            std::unique_ptr<downloader::FileDownloader> downloader,
            std::unique_ptr<settings::Settings> app_settings) :
    context_(context),
    writer_(std::move(writer)),
    downloader_(std::move(downloader)),
    settings_(std::move(app_settings)) {}

    Processor::~Processor() = default;

    void Processor::process() {
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
