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

    struct tensor {
        std::vector<double> data;
        std::vector<uintptr_t> shape;
    };

    inline auto to_array_view_state_impl(const tensor& t) {
        return questdb::ingress::array::row_major_view<double>{
            t.shape.size(),
            t.shape.data(),
            t.data.data(),
            t.data.size(),
        };
    }

    inline tensor to_tensor(const std::vector<models::PriceLevel> &price_levels) {
        // price level is a price and a volume as a double
        // so two columns, n rows
        //  would be the number of price levels - so the length of the vector
        [[likely]] if (size_t size = price_levels.size(); size > 0) {
            tensor t;
            t.shape = {size, 2};
            t.data.reserve(size * 2);
            for (const auto &[price, quantity] : price_levels) {
                t.data.push_back(price);
                t.data.push_back(quantity);
            }
            return t;
        }
        return tensor{};
    }

    class QuestDBWriter final : IWriter {
        moodycamel::ConcurrentQueue<models::DataEvent> &buffer_;
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

        explicit QuestDBWriter(moodycamel::ConcurrentQueue<models::DataEvent> &buffer,
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

        void writeTradeToDbBuffer(const models::Trade& trade_event);
        void writeCandleToDbBuffer(const models::Candle& candle_event);
        void writeOrderbookToDbBuffer(const models::OrderbookSnapshot& orderbook_event);
    };
}
