//
// Created by jtwears on 9/14/25.
//

#include <chrono>
#include <string>
#include <memory>
#include <questdb/ingress/line_sender.hpp>

#include "binancehistoricaldatafetcher/questdb_writer.h"
#include "binancehistoricaldatafetcher/file_downloader.h"
#include "binancehistoricaldatafetcher/settings.h"
#include "binancehistoricaldatafetcher/processor.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"


using namespace std::chrono_literals;

namespace writer {

    QuestDBWriter::QuestDBWriter(moodycamel::ConcurrentQueue<models::DataEvent> &buffer,
        const std::string &dbConnectionURI,
        const std::shared_ptr<processor::Context> &context,
        const std::shared_ptr<std::unordered_map<std::string, models::ExchangeInfo>> &exchangeInfo,
        const int batchSize,
        const int flushIntervalMs,
        const settings::DataType dataType) : buffer_(buffer),
                                            dbConnectionURI(dbConnectionURI),
                                            batchSize_(batchSize),
                                            dbSender(questdb::ingress::line_sender::from_conf(dbConnectionURI)),
                                            flushIntervalMs_(flushIntervalMs),
                                            context_(context),
                                            dataType_(dataType),
                                            dbBuffer_(dbSender.new_buffer()),
                                            flushInterval_(flushIntervalMs * 1ms),
                                            exchangeInfo_(exchangeInfo)
    {}


    void QuestDBWriter::close() {
       if (getEventsWritten() > 0) {
           dbSender.flush(dbBuffer_);
       }
        dbSender.close();
        context_.get()->consumerDone.store(true);
        context_.get()->running.store(false);
    }

    void QuestDBWriter::write() {
        const auto now = steady_clock::now;
        while (context_.get()->running.load()) {
            models::DataEvent event;
            auto start = now();
            while (context_.get()->running.load()) {

                if (!buffer_.try_dequeue(event)) {
                   if (context_.get()->producerDone.load()) {
                       break;
                   }
                    std::this_thread::yield();
                    continue;
                }

                switch (dataType_) {
                   case settings::TRADES:
                       writeTradeToDbBuffer(*event.futures_trade);
                       break;
                    case settings::OHLCV:
                       writeCandleToDbBuffer(*event.candle);
                       break;
                    case settings::SNAPSHOT:
                       writeOrderbookToDbBuffer(*event.orderbook_snapshot);
                       break;
                    default:
                       close();
                       throw std::runtime_error("Unknown data type");
               }
                incrementEventsWritten(1);
                if (getEventsWritten() >= batchSize_ || (steady_clock::now() - start) > flushInterval_) {
                    break;
                }
            }

            if (getEventsWritten() > 0) {
                dbSender.flush(dbBuffer_);
                resetEventsWritten();
            }

            if (!buffer_.try_dequeue(event) && context_.get()->producerDone.load()) {
                close();
                return;
            }
        }
    }

    void QuestDBWriter::writeCandleToDbBuffer(const models::Candle& candle_event) {
        const auto openTimeAt = questdb::ingress::timestamp_micros(candle_event.open_time);
        dbBuffer_.table("candles")
        .symbol("symbol", candle_event.symbol)
        .symbol("product_type", getProductName(candle_event.product_type))
        .symbol("frequency", getCandleFrequencyName(candle_event.frequency))
        .column("open_time", candle_event.open_time)
        .column("open", candle_event.open)
        .column("high", candle_event.high)
        .column("low", candle_event.low)
        .column("close", candle_event.close)
        .column("volume", candle_event.volume)
        .column("close_time", candle_event.close_time)
        .at(openTimeAt);
    }

    void QuestDBWriter::writeTradeToDbBuffer(const models::Trade& trade_event) {
        const auto tradeTimeAt = questdb::ingress::timestamp_micros(trade_event.time);
        const auto side = models::sideToString(trade_event.side);
        dbBuffer_.table("trades")
        .symbol("symbol", trade_event.symbol)
        .symbol("side", side)
        .symbol("product_type", settings::getProductName(trade_event.product_type))
        .column("id", trade_event.id)
        .column("price", trade_event.price)
        .column("volume", trade_event.qty)
        .column("quote_volume", trade_event.quote_qty)
        .at(tradeTimeAt);
    }

    void QuestDBWriter::writeOrderbookToDbBuffer(const models::OrderbookSnapshot& orderbook_event) {
        auto [tick_size, step_size] = exchangeInfo_->at(orderbook_event.symbol);
        const auto bids = to_tensor(orderbook_event.bids, tick_size, step_size);
        const auto asks = to_tensor(orderbook_event.asks, tick_size, step_size);
        dbBuffer_.table("binance_snapshots")
        .symbol("symbol", orderbook_event.symbol)
        .symbol("product_type", settings::getProductName(orderbook_event.product_type))
        .column("bids", bids)
        .column("asks", asks)
        .at_now();

    }
}