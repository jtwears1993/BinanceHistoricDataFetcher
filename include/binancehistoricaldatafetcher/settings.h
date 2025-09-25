//
// Created by jtwears on 9/14/25.
//

#pragma once

#include <vector>
#include <string>
#include <optional>


namespace settings {
    enum Product {
        FUTURES,
    };

    Product getProduct(const std::string &productName);

    std::string getProductName(Product product);

    enum DownloadType {
        MONTHLY,
        DAILY,
    };

    DownloadType getDownloadType(const std::string &downloadTypeName);

    std::string getDownloadTypeName(DownloadType downloadType);

    enum OutputType {
        PARQUET,
        QUESTDB,
    };

    OutputType getOutputType(const std::string &outputTypeName);

    std::string getOutputTypeName(OutputType outputType);

    enum DataType {
        TRADES,
        OHLCV,
    };

    DataType getDataType(const std::string &dataType);

    std::string getDataTypeName(DataType dataType);

    enum CandleFrequency {
        ONE_MINUTE,
        THREE_MINUTES,
        FIVE_MINUTES,
        FIFTEEN_MINUTES,
        THIRTY_MINUTES,
        ONE_HOUR,
        TWO_HOURS,
        FOUR_HOURS,
        SIX_HOURS,
        EIGHT_HOURS,
        TWELVE_HOURS,
        ONE_DAY,
        THREE_DAYS,
        ONE_WEEK,
        ONE_MONTH
    };

    CandleFrequency getCandleFrequency(const std::string &frequency);

    std::string getCandleFrequencyName(CandleFrequency frequency);

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