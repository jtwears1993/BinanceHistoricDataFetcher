//
// Created by jtwears on 9/15/25.
//

#pragma once

#include <atomic>
#include <memory>

#include "settings.h"

namespace downloader { class FileDownloader; }
namespace writer { class QuestDBWriter; }

namespace processor {

    struct Context {
        std::atomic<bool> producerDone{false};
        std::atomic<bool> consumerDone{false};
        std::atomic<bool> running{true};
    };

    class Processor {
        std::shared_ptr<Context> context_;
        std::unique_ptr<writer::QuestDBWriter> writer_;
        std::unique_ptr<downloader::FileDownloader> downloader_;
        std::unique_ptr<settings::Settings> settings_;

    public:
        explicit Processor(const std::shared_ptr<Context> &context,
            std::unique_ptr<writer::QuestDBWriter> writer,
            std::unique_ptr<downloader::FileDownloader> downloader,
            std::unique_ptr<settings::Settings> app_settings);
        ~Processor();

        void process();
    };
}


