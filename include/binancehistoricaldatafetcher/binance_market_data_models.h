//
// Created by jtwears on 9/27/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_MARKET_DATA_MODELS_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_MARKET_DATA_MODELS_H

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

#include "constants.h"
#include "settings.h"
#include "common/rounding/fixed_point.h"


namespace models {

    enum BinanceStreams {
        PARTIAL_DEPTH,
        TRADES
    };

    enum Side {
        BUY,
        SELL
    };

    inline std::string sideToString(const Side side) {
        return side == BUY ? "BUY" : "SELL";
    }

    inline Side getTradeSide(const bool isBuyerMaker) {
        return isBuyerMaker ? BUY : SELL;
    }

    inline std::string getFuturesUrl(const std::string &symbol, const std::string &downloadType, const std::string &dataType) {
        return std::string(settings::BASE_URL) + std::string(settings::FUTURES_BASE) + downloadType + "/" + dataType + "/" + symbol + "/";
    }


    inline std::string getFileName(const std::string &symbol, const std::string &date, const std::string &dataType) {
        return symbol + "-" + dataType + + "-" + date + ".zip";
    }

    struct Trade {
        int64_t id;
        double price;
        double qty;
        double quote_qty;
        int64_t time;
        Side side;
        std::string symbol;
        settings::Product product_type;
    };


    struct Candle {
        int64_t open_time;
        double open;
        double high;
        double low;
        double close;
        double volume;
        int64_t close_time;
        std::string symbol;
        settings::Product product_type;
        settings::CandleFrequency frequency;
    };

    struct PriceLevel {
        std::int32_t price;
        std::int32_t quantity;
    };

    inline void from_json(const nlohmann::json &j, PriceLevel &p, const int price_precision, const int quantity_precision) {
        if (j.is_array() && j.size() == 2) {
            p.price = common::rounding::FixedPoint::from_string(j.at(0).get<std::string>(), price_precision);
            p.quantity = common::rounding::FixedPoint::from_string(j.at(1).get<std::string>(), quantity_precision);
        } else {
            throw std::runtime_error("Invalid PriceLevel JSON format");
        }
    }

    struct BinanceFuturesOrderbookSnapshot {
        unsigned long long lastUpdate_id; // "lastUpdateId"
        long long message_time;    // "E"
        long long transaction_time; // "T"
        std::vector<PriceLevel> bids; // "bids"
        std::vector<PriceLevel> asks; // "asks"
        std::string symbol;
    };

    inline void from_json(const nlohmann::json &j, BinanceFuturesOrderbookSnapshot &snapshot, const int price_precision, const int quantity_precision) {
        j.at("lastUpdateId").get_to(snapshot.lastUpdate_id);
        j.at("E").get_to(snapshot.message_time);
        j.at("T").get_to(snapshot.transaction_time);

        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
        for (const auto &item : j.at("bids")) {
            PriceLevel p{};
            from_json(item, p, price_precision, quantity_precision);
            bids.push_back(p);
        }
        for (const auto &item : j.at("asks")) {
            PriceLevel p{};
            from_json(item, p, price_precision, quantity_precision);
            asks.push_back(p);
        }
        snapshot.bids = bids;
        snapshot.asks = asks;
    }

    struct BinanceFuturesSocketDepthSnapshot {
        std::string event_type;          // "e"
        long long event_time;            // "E"
        long long transaction_time;      // "T"
        std::string symbol;             // "s"
        unsigned long long first_update_id; // "U"
        unsigned long long final_update_id; // "u"
        unsigned long long previous_final_update_id; // "pu"
        std::vector<PriceLevel> bids; // "b"
        std::vector<PriceLevel> asks; // "a"
    };

    // inline void from_json(const nlohmann::json &j, BinanceFuturesSocketDepthSnapshot &snapshot) {
    //     j.at("e").get_to(snapshot.event_type);
    //     j.at("E").get_to(snapshot.event_time);
    //     j.at("T").get_to(snapshot.transaction_time);
    //     j.at("s").get_to(snapshot.symbol);
    //     j.at("U").get_to(snapshot.first_update_id);
    //     j.at("u").get_to(snapshot.final_update_id);
    //     j.at("pu").get_to(snapshot.previous_final_update_id);
    //     j.at("b").get_to(snapshot.bids);
    //     j.at("a").get_to(snapshot.asks);
    // }

    struct ExchangeInfo {
        int tick_size;
        int step_size;
    };

    struct OrderbookSnapshot {
        long long snapshot_time;
        std::string symbol;
        settings::Product product_type;
        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
    };

    struct DataEvent {
        std::optional<Trade> futures_trade;
        std::optional<Candle> candle;
        std::optional<OrderbookSnapshot > orderbook_snapshot;
    };

    struct BinanceFuturesOnOpenSocketMessage {
        std::string method;
        std::vector<std::string> params;
        int id;
    };

}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_MARKET_DATA_MODELS_H