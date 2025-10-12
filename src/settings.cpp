//
// Created by jtwears on 9/14/25.
//

#include <string>
#include <stdexcept>
#include "binancehistoricaldatafetcher/settings.h"

namespace settings {
    Product getProduct(const std::string &productName) {
        if (productName == "futures") {
            return FUTURES;
        }
        throw std::invalid_argument("Invalid product name: " + productName);
    }

    std::string getProductName(const Product product) {
        switch (product) {
            case FUTURES:
                return "futures";
            default:
                throw std::invalid_argument("Invalid product enum value");
        }
    }

    DownloadType getDownloadType(const std::string &downloadTypeName) {
        if (downloadTypeName == "monthly") {
            return MONTHLY;
        }
        if (downloadTypeName == "daily") {
            return DAILY;
        }
        throw std::invalid_argument("Invalid download type name: " + downloadTypeName);
    }

    std::string getDownloadTypeName(const DownloadType downloadType) {
        switch (downloadType) {
            case MONTHLY:
                return "monthly";
            case DAILY:
                return "daily";
            default:
                throw std::invalid_argument("Invalid download type enum value");
        }
    }

    OutputType getOutputType(const std::string &outputTypeName) {
        if (outputTypeName == "parquet") {
            return PARQUET;
        }
        if (outputTypeName == "questdb") {
            return QUESTDB;
        }
        throw std::invalid_argument("Invalid output type name: " + outputTypeName);
    }

    std::string getOutputTypeName(const OutputType outputType) {
        switch (outputType) {
            case PARQUET:
                return "parquet";
            case QUESTDB:
                return "questdb";
            default:
                throw std::invalid_argument("Invalid output type enum value");
        }
    }

    CandleFrequency getCandleFrequency(const std::string &frequency) {
        if (frequency == "1m") return ONE_MINUTE;
        if (frequency == "3m") return THREE_MINUTES;
        if (frequency == "5m") return FIVE_MINUTES;
        if (frequency == "15m") return FIFTEEN_MINUTES;
        if (frequency == "30m") return THIRTY_MINUTES;
        if (frequency == "1h") return ONE_HOUR;
        if (frequency == "2h") return TWO_HOURS;
        if (frequency == "4h") return FOUR_HOURS;
        if (frequency == "6h") return SIX_HOURS;
        if (frequency == "8h") return EIGHT_HOURS;
        if (frequency == "12h") return TWELVE_HOURS;
        if (frequency == "1d") return ONE_DAY;
        if (frequency == "3d") return THREE_DAYS;
        if (frequency == "1w") return ONE_WEEK;
        if (frequency == "1M") return ONE_MONTH;

        throw std::invalid_argument("Invalid candle frequency: " + frequency);
    }

    std::string getCandleFrequencyName(const CandleFrequency candleFrequency) {
        switch (candleFrequency) {
            case ONE_MINUTE:
                return "1m";
            case THREE_MINUTES:
                return "3m";
            case FIVE_MINUTES:
                return "5m";
            case FIFTEEN_MINUTES:
                return "15m";
            case THIRTY_MINUTES:
                return "30m";
            case ONE_HOUR:
                return "1h";
            case TWO_HOURS:
                return "2h";
            case FOUR_HOURS:
                return "4h";
            case SIX_HOURS:
                return "6h";
            case EIGHT_HOURS:
                return "8h";
            case TWELVE_HOURS:
                return "12h";
            case ONE_DAY:
                return "1d";
            case THREE_DAYS:
                return "3d";
            case ONE_WEEK:
                return "1w";
            case ONE_MONTH:
                return "1M";
            default:
                throw std::invalid_argument("Invalid candle frequency enum value");
        }
    }

    std::string getDataTypeName(const DataType dataType) {
        switch (dataType) {
            case TRADES:
                return "trades";
            case OHLCV:
                return "ohlcv";
            case SNAPSHOT:
                return "snapshot";
            default:
                throw std::invalid_argument("Invalid data type enum value");
        }
    }

    DataType getDataType(const std::string &dataType) {
        if (dataType == "trades") {
            return TRADES;
        }
        if (dataType == "ohlcv") {
            return OHLCV;
        }
        if (dataType == "snapshot") {
            return SNAPSHOT;
        }
        throw std::invalid_argument("Invalid data type name: " + dataType);
    }
}