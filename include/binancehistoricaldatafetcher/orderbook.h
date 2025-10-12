//
// Created by jtwears on 9/27/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_ORDERBOOK_H
#define BINANCEHISTORICDATAFETCHER_ORDERBOOK_H

#include <map>
#include <memory_resource>
#include <ranges>
#include <vector>

#include "binance_market_data_models.h"

namespace models {

    struct BidComparator {
        bool operator()(const double a, const double b) const {
            return a > b; // Higher prices first
        }
    };

    struct AskComparator {
        bool operator()(const double a, const double b) const {
            return a < b; // Lower prices first
        }
    };

    class Orderbook {
        std::pmr::map<double, PriceLevel, BidComparator> bids_; // price -> quantity
        std::pmr::map<double, PriceLevel, AskComparator> asks_; // price -> quantity

    public:
        Orderbook() : bids_(
            BidComparator(), std::pmr::get_default_resource()),
            asks_(AskComparator(), std::pmr::get_default_resource()) {}
        ~Orderbook() = default;

        [[nodiscard]] std::vector<PriceLevel> get_bids() const& {
            std::vector<PriceLevel> bids;
            for (const auto &level: bids_ | std::views::values) {
                bids.push_back(level);
            }
            return bids;
        }

        [[nodiscard]] std::vector<PriceLevel> get_asks() const& {
            std::vector<PriceLevel> asks;
            for (const auto &level: asks_ | std::views::values) {
                asks.push_back(level);
            }
            return asks;
        }
        // index 0 = best bid, index 1 = best ask
        [[nodiscard]] std::tuple<PriceLevel, PriceLevel> get_top_of_book() const {
            std::tuple<PriceLevel, PriceLevel> top_of_book;
            if (!bids_.empty()) {
                std::get<0>(top_of_book) = bids_.begin()->second;
            } else {
                std::get<0>(top_of_book) = PriceLevel{0.0, 0.0};
            }
            if (!asks_.empty()) {
                std::get<1>(top_of_book) = asks_.begin()->second;
            } else {
                std::get<1>(top_of_book) = PriceLevel{0.0, 0.0};
            }
            return top_of_book;
        }
        // index 0 = bids, index 1 = asks
        [[nodiscard]] std::tuple<std::vector<PriceLevel>, std::vector<PriceLevel>> get_levels(const size_t depth) const {
            std::tuple<std::vector<PriceLevel>, std::vector<PriceLevel>> levels;
            std::vector<PriceLevel> bids;
            std::vector<PriceLevel> asks;

            size_t count = 0;
            for (const auto &level: bids_ | std::views::values) {
                if (count >= depth) break;
                bids.push_back(level);
                count++;
            }
            count = 0;
            for (const auto &level: asks_ | std::views::values) {
                if (count >= depth) break;
                asks.push_back(level);
                count++;
            }
            std::get<0>(levels) = bids;
            std::get<1>(levels) = asks;
            return levels;
        }

        void add(const double price, const double volume, const bool is_bid) {

            if (!is_valid_price(price) || volume <= 0.0) {
                return; // Invalid price or volume
            }

            // if price level already exists, update quantity
            if (is_bid) {
                if (const auto it = bids_.find(price); it != bids_.end()) {
                    it->second.quantity += volume;
                    return; // Successfully updated
                }
                bids_.emplace(price, PriceLevel{price, volume});
                return; // Successfully added
            }
            if (const auto it = asks_.find(price); it != asks_.end()) {
                it->second.quantity += volume;
                return;
            }
            asks_.emplace(price, PriceLevel{price, volume});
        }

        int remove(const double price, const bool is_bid) {

            if (!is_valid_price(price)) {
                return -2; // Invalid price
            }

            if (is_bid) {
                if (const int res = bids_.erase(price); res == 0) {
                    return -1; // Price level not found
                }
                return 0; // Successfully removed
            }
            if (const int res = asks_.erase(price); res == 0) {
                return -1;
            }
            return 0;
        }

    private:
        static bool is_valid_price(const double price) {
            return price > 0.0;
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_ORDERBOOK_H