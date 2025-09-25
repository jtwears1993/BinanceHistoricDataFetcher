//
// Created by jtwears on 9/14/25.
//

# pragma once


#include <string>
#include <memory>
#include <chrono>
#include <atomic>
#include "libs/concurrentqueue/concurrentqueue.h"
#include <questdb/ingress/line_sender.hpp>

#include "file_downloader.h"
#include "writer.h"
#include "settings.h"
#include "processor.h"


using namespace std;
using namespace std::chrono;

namespace processor { struct Context; }

namespace writer {
    class QuestDBWriter final : IWriter {
        moodycamel::ConcurrentQueue<downloader::DataEvent> &buffer_;
        string dbConnectionURI;
        int batchSize_{};
        questdb::ingress::line_sender dbSender;
        int flushIntervalMs_;
        const std::shared_ptr<processor::Context> context_;
        std::atomic<int> eventsWritten_{0};
        settings::DataType dataType_;
        questdb::ingress::line_sender_buffer dbBuffer_;
        milliseconds flushInterval_;


    public:

        explicit QuestDBWriter(moodycamel::ConcurrentQueue<downloader::DataEvent> &buffer,
            const std::string &dbConnectionURI,
            const std::shared_ptr<processor::Context> &context,
            int batchSize = 1000,
            int flushIntervalMs = 1000,
            settings::DataType dataType = settings::TRADES);

        void write() override;

        void close() override;

    private:
        void incrementEventsWritten(const int count) {
            eventsWritten_ += count;
        }

        void resetEventsWritten() {
            eventsWritten_ = 0;
        }

        int getEventsWritten() const {
            return eventsWritten_;
        }

        void writeTradeToDbBuffer(const downloader::Trade& trade_event);
        void writeCandleToDbBuffer(const downloader::Candle& candle_event);
    };
}
