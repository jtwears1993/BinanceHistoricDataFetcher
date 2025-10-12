//
// Created by jtwears on 9/29/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_ORDERBOOK_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_ORDERBOOK_H
#include <string>
#include <vector>

#include "binancehistoricaldatafetcher/binance_market_data_models.h"
#include "binancehistoricaldatafetcher/multi_symbol_orderbook.h"
#include "binancehistoricaldatafetcher/settings.h"

namespace models {
    template<typename UpdateMsg, typename SnapshotMsg>
    class BinanceOrderbook  {
    protected:
        MultiSymbolOrderbook multi_symbol_orderbook_;
        const std::vector<std::string> symbols_;
        const settings::Product product_;


    public:
        explicit BinanceOrderbook(const std::vector<std::string>& symbols, const settings::Product product) :
            multi_symbol_orderbook_(symbols), symbols_(symbols), product_(product) {};

        virtual ~BinanceOrderbook() = default;

        virtual void init(const std::vector<SnapshotMsg>& snapshots) = 0;

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