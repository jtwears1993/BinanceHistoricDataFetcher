//
// Created by jtwears on 11/5/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_SETTINGS_H
#define BINANCEHISTORICDATAFETCHER_SETTINGS_H

#include <string>
#include <optional>
#include <vector>
#include "common/models/enums.h"

namespace binance::settings {

    using namespace common::models::enums;

    struct Settings {
        std::string startDate;
        std::string endDate;
        Product product;
        DownloadType downloadType;
        OutputType outputType;
        DataType dataType;
        int batchSize;
        std::optional<std::string> dbUrl; // Only for QuestDB
        std::optional<std::string> outputDir; // Only for Parquet
        std::optional<CandleFrequency> candleFrequency;
        std::vector<std::string> symbols;
    };
}

#endif //BINANCEHISTORICDATAFETCHER_SETTINGS_H