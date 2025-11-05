//
// Created by jtwears on 9/14/25.
//

#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include <chrono>

#include "binance_market_data_models.h"
#include "concurrentqueue/concurrentqueue.h"

#include "common/models/enums.h"

using namespace common::models;
using namespace common::models::enums;

// forward declare to prevent circular import
namespace common::sync::producer_consumer { struct Context; }

namespace downloader {

    class FileDownloader {
        moodycamel::ConcurrentQueue<DataEvent> &queue_;
        std::shared_ptr<common::sync::producer_consumer::Context> &context_;
        std::filesystem::path tmp_dir_;
        std::filesystem::path tmp_dir_file_ ;
        const DataType data_type_;
        const Product product_type_;
        const DownloadType download_type_;

    public:
        FileDownloader(
            DataType dataType,
            Product productType,
            DownloadType downloadType,
            moodycamel::ConcurrentQueue<DataEvent> &queue,
            std::shared_ptr<common::sync::producer_consumer::Context> &context);
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