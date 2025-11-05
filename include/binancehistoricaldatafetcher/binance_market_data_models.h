//
// Created by jtwears on 9/27/25.
//

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "constants.h"
#include "common/models/enums.h"
#include "common/models/common_data_models.h"

using namespace common::models::enums;
using namespace common::models;
using namespace binance::settings;

namespace binance::models {

    enum BinanceStreams {
        PARTIAL_DEPTH,
        TRADES
    };


    inline std::string getFuturesUrl(const std::string &symbol, const std::string &downloadType, const std::string &dataType) {
        return std::string(BASE_URL) + std::string(FUTURES_BASE) + downloadType + "/" + dataType + "/" + symbol + "/";
    }


    inline std::string getFileName(const std::string &symbol, const std::string &date, const std::string &dataType) {
        return symbol + "-" + dataType + + "-" + date + ".zip";
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

    struct BinanceFuturesOnOpenSocketMessage {
        std::string method;
        std::vector<std::string> params;
        int id;
    };
}