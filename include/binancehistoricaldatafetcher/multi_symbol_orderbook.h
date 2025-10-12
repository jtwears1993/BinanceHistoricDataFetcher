//
// Created by jtwears on 9/27/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_ORDERBOOK_MANAGER_H
#define BINANCEHISTORICDATAFETCHER_ORDERBOOK_MANAGER_H

#include <map>
#include <string>

#include "orderbook.h"

namespace models {
    class MultiSymbolOrderbook {
        std::map<std::string, Orderbook> multi_symbol_orderbook_;

    public:
        explicit MultiSymbolOrderbook(const std::vector<std::string>& symbols) {
            for (const auto& symbol : symbols) {
                multi_symbol_orderbook_.emplace(symbol, Orderbook());
            }
        }

        ~MultiSymbolOrderbook() = default;


        std::tuple<PriceLevel, PriceLevel> get_top_of_book(const std::string &symbol) {
            const auto book = get_book(symbol);
            if (book == nullptr) {
                throw std::invalid_argument("Symbol not found in orderbook manager");
            }
            return book->get_top_of_book();

        }

        std::tuple<std::vector<PriceLevel>, std::vector<PriceLevel>> get_levels(const std::string &symbol, const size_t depth) {

            const auto book = get_book(symbol);
            if (book == nullptr) {
                throw std::invalid_argument("Symbol not found in orderbook manager");
            }
            return book->get_levels(depth);
        }

        bool has_book(const std::string &symbol) const {
            const auto exists = multi_symbol_orderbook_.find(symbol);
            return exists != multi_symbol_orderbook_.end();
        }

        void remove_book(const std::string &symbol) {
           const auto book = get_book(symbol);
            if (book == nullptr) {
                return;
            }
            multi_symbol_orderbook_.erase(symbol);
        }

        void update_price_level(const std::string &symbol, const PriceLevel price_level, const bool is_bid) {
            const auto  book = get_book(symbol);
            if (book == nullptr) {
                return;
            }

            if (is_bid) {
                book->add(price_level.price, price_level.quantity, true);
            }
            else {
                book->add(price_level.price, price_level.quantity, false);
            }
        }

        void remove_price_level(const std::string &symbol, const PriceLevel &priceLevel, const bool is_bid) {
            const auto book = get_book(symbol);
            if (book == nullptr) {
                return;
            }
            book->remove(priceLevel.price, is_bid);
        }

    private:
        // I want to force users to use the public methods to access the orderbook
        // to ensure that the symbol exists and to control the editing and ownership of the orderbook
        // objects
        Orderbook* get_book(const std::string &symbol) {
            if (const auto exists = multi_symbol_orderbook_.find(symbol); exists != multi_symbol_orderbook_.end()) {
                return &exists->second;
            }
            return nullptr;
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_ORDERBOOK_MANAGER_H