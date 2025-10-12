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
#include "../include/libs/concurrentqueue/concurrentqueue.h"

#include "settings.h"
#include "constants.h"

using namespace std;

// forward declare to prevent circular import
namespace processor { struct Context; }

namespace downloader {


    class FileDownloader {
        moodycamel::ConcurrentQueue<models::DataEvent> &queue_;
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
            moodycamel::ConcurrentQueue<models::DataEvent> &queue,
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