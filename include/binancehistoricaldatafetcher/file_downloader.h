//
// Created by jtwears on 9/14/25.
//

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <filesystem>
#include <vector>
#include <chrono>

#include "../include/libs/concurrentqueue/concurrentqueue.h"

#include "settings.h"
#include "constants.h"

using namespace std;

// forward declare to prevent circular import
namespace processor { struct Context; }

namespace downloader {
    enum Side {
        BUY,
        SELL
    };

    inline std::string sideToString(const Side side) {
        return side == BUY ? "BUY" : "SELL";
    }

    inline Side getTradeSide(const bool isBuyerMaker) {
        return isBuyerMaker ? BUY : SELL;
    }

    inline std::string getFuturesUrl(const std::string &symbol, const std::string &downloadType, const std::string &dataType) {
        return std::string(settings::BASE_URL) + std::string(settings::FUTURES_BASE) + downloadType + "/" + dataType + "/" + symbol + "/";
    }


    inline std::string getFileName(const std::string &symbol, const std::string &date, const std::string &dataType) {
        return symbol + "-" + dataType + + "-" + date + ".zip";
    }

    struct Trade {
        int64_t id;
        double price;
        double qty;
        double quote_qty;
        int64_t time;
        Side side;
        std::string symbol;
        settings::Product product_type;
    };


    struct Candle {
        int64_t open_time;
        double open;
        double high;
        double low;
        double close;
        double volume;
        int64_t close_time;
        std::string symbol;
        settings::Product product_type;
        settings::CandleFrequency frequency;
    };

    struct DataEvent {
        std::optional<Trade> futures_trade;
        std::optional<Candle> candle;
    };

    class FileDownloader {
        moodycamel::ConcurrentQueue<DataEvent> &queue_;
        std::shared_ptr<processor::Context> &context_;
        std::filesystem::path tmp_dir_;
        std::filesystem::path tmp_dir_file_ ;
        const settings::DataType data_type_;
        const settings::Product product_type_;
        const settings::DownloadType download_type_;

    public:
        FileDownloader(
            settings::DataType dataType,
            settings::Product productType,
            settings::DownloadType downloadType,
            moodycamel::ConcurrentQueue<DataEvent> &queue,
            std::shared_ptr<processor::Context> &context);
        ~FileDownloader();
        void download(const std::vector<std::string> &symbol, const std::string &start_date, const std::string &end_date) const;

    private:
        [[nodiscard]] bool downloadFile(const std::string &url) const;
        [[nodiscard]] bool unzipFile() const;
        void readFuturesTradeFile(const std::string& url, const std::string& symbol) const;
        void readCandleFile(const std::string& url, const std::string& symbol) const;
        void deleteFile() const;
        [[nodiscard]] std::vector<std::string> createUrls(const std::string &symbol, const std::string &start_date, const std::string &end_date) const;
        static std::chrono::year_month_day parseDateString(const std::string &dateString);
    };
}