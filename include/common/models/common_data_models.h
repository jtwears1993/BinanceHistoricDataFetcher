//
// Created by jtwears on 10/31/25.
//
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

#include "common/rounding/fixed_point.h"
#include "common/models/enums.h"

namespace common::models {

    struct PriceLevel {
        std::int32_t price;
        std::int32_t quantity;
    };

    inline void from_json(const nlohmann::json &j, PriceLevel &p, const int price_precision, const int quantity_precision) {
        if (j.is_array() && j.size() == 2) {
            p.price = rounding::FixedPoint::from_string(j.at(0).get<std::string>(), price_precision);
            p.quantity = rounding::FixedPoint::from_string(j.at(1).get<std::string>(), quantity_precision);
        } else {
            throw std::runtime_error("Invalid PriceLevel JSON format");
        }
    }

    inline void to_json(nlohmann::json &j, const PriceLevel &p) {
        j = {
            "price", p.price,
            "quantity", p.quantity
        };
    }


    struct ExchangeInfo {
        int tick_size;
        int step_size;
    };

    struct Trade {
        int64_t id;
        double price;
        double qty;
        double quote_qty;
        int64_t time;
        enums::Side side;
        std::string symbol;
        enums::Product product_type;
    };

    inline void to_json(nlohmann::json &j, const Trade &t) {
        j = {
            {"id", t.id},
            {"price", t.price},
            {"qty", t.qty},
            {"quote_qty", t.quote_qty},
            {"time", t.time},
            {"side", sideToString(t.side)},
            {"symbol", t.symbol},
            {"product_type", t.product_type}
        };
    }


    struct Candle {
        int64_t open_time;
        double open;
        double high;
        double low;
        double close;
        double volume;
        int64_t close_time;
        std::string symbol;
        enums::Product product_type;
        enums::CandleFrequency frequency;
    };

    inline void to_json(nlohmann::json &j, const Candle &c) {}

    struct OrderbookSnapshot {
        long long snapshot_time;
        std::string symbol;
        enums::Product product_type;
        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
    };

    inline void to_json(nlohmann::json &j, const OrderbookSnapshot &snapshot) {
        j = {
            {"snapshot_time", snapshot.snapshot_time},
            {"symbol", snapshot.symbol},
            {"product_type", snapshot.product_type},
            {"bids", snapshot.bids},
            {"asks", snapshot.asks}
        };
    }

    struct DataEvent {
        std::optional<Trade> futures_trade;
        std::optional<Candle> candle;
        std::optional<OrderbookSnapshot > orderbook_snapshot;
    };

    inline void to_json(nlohmann::json &j, const DataEvent &event) {

        if (event.orderbook_snapshot.has_value()) {
            j["snapshot"] = event.orderbook_snapshot.value();
        }

        if (event.futures_trade.has_value()) {
            j["futures_trade"] = event.futures_trade.value();
        }

        if (event.candle.has_value()) {
            j["candle"] = event.candle.value();
        }
    }
}