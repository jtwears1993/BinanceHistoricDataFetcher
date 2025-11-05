//
// Created by jtwears on 9/29/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_ORDERBOOK_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_ORDERBOOK_H
#include <string>
#include <vector>

#include "common/models/multi_symbol_orderbook.h"
#include "common/models/enums.h"

using namespace common::models;
using namespace common::models::enums;

namespace binance::models {
    template<typename UpdateMsg>
    class BinanceOrderbook  {
    protected:
        MultiSymbolOrderbook multi_symbol_orderbook_;
        const std::vector<std::string> symbols_;
        const Product product_;
        std::shared_ptr<std::unordered_map<std::string, ExchangeInfo>> exchange_info_;

    public:
        explicit BinanceOrderbook(const std::vector<std::string>& symbols,
            const Product product,
            const std::shared_ptr<std::unordered_map<std::string, ExchangeInfo>> &exchange_info) :
            multi_symbol_orderbook_(symbols), symbols_(symbols), product_(product),
            exchange_info_(exchange_info) {};

        virtual ~BinanceOrderbook() = default;

        virtual void init(const std::vector<std::string>& symbols) = 0;

        virtual int process_update(const UpdateMsg& snapshot) = 0;

        OrderbookSnapshot get_snapshot(const std::string& symbol, const size_t depth) {
            const auto levels = multi_symbol_orderbook_.get_levels(symbol, depth);
            // Construct the OrderbookSnapshot
            OrderbookSnapshot snapshot;
            snapshot.symbol = symbol;
            snapshot.bids = std::get<0>(levels);
            snapshot.asks = std::get<1>(levels);
            // set snapshot time to current time in milliseconds
            snapshot.snapshot_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            snapshot.product_type = product_;
            return snapshot;
        }

        [[nodiscard]] virtual bool is_initialized(const std::string &symbol) const {
            return false;
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_ORDERBOOK_H