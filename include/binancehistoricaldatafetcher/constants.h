//
// Created by jtwears on 9/15/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_CONSTANTS_H
#define BINANCEHISTORICDATAFETCHER_CONSTANTS_H
namespace settings {
    constexpr auto APP_VERSION = "0.0.1";
    constexpr auto APP_NAME = "BinanceHistoricDataFetcher";
    constexpr auto APP_DESCRIPTION = "A tool to fetch and store historical data from Binance. Futures Supported Only in v0.0.1";
    constexpr auto BASE_URL = "https://data.binance.vision/";
    constexpr auto FUTURES_BASE = "/?prefix=data/futures/um/";
    constexpr auto TRADE_URL = "/trades";
    constexpr auto OHLCV_URL = "/klines";
    constexpr auto BATCH_SIZE = 5000;
    constexpr auto BUFFER_SIZE = 250000;
    constexpr auto FLUSH_INTERVAL_MS = 2000;
}
#endif //BINANCEHISTORICDATAFETCHER_CONSTANTS_H