//
// Created by jtwears on 9/14/25.
//


#include <memory>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>

#include <cpr/cpr.h>
#include <elzip/elzip.hpp>

#include "binancehistoricaldatafetcher/file_downloader.h"
#include "binancehistoricaldatafetcher/processor.h"
#include "binancehistoricaldatafetcher/settings.h"
#include "binancehistoricaldatafetcher/binance_market_data_models.h"

using namespace std;

namespace downloader {

    FileDownloader::FileDownloader(
        const settings::DataType dataType,
        const settings::Product productType,
        const settings::DownloadType downloadType,
        moodycamel::ConcurrentQueue<models::DataEvent> &queue,
        std::shared_ptr<processor::Context> &context) :
        queue_(queue),
        context_(context),
        tmp_dir_(std::filesystem::temp_directory_path()),
        data_type_(dataType),
        product_type_(productType),
        download_type_(downloadType) {

        const auto tm_dir_path = tmp_dir_ / "tmp-historical-binance-data";
        if (!std::filesystem::exists(tm_dir_path)) {
            std::filesystem::create_directory(tm_dir_path);
        }
        tmp_dir_file_ = tm_dir_path;
   }

    FileDownloader::~FileDownloader() {
        if (std::filesystem::exists(tmp_dir_file_)) {
            std::filesystem::remove_all(tmp_dir_file_);
        }
    }

    void FileDownloader::download(const std::vector<std::string> &symbols, const std::string &start_date, const std::string &end_date) const {
        for (const std::string& symbol : symbols) {
            for (const std::vector<std::string> urls = createUrls(symbol, start_date, end_date); const auto &url : urls) {
                if (downloadFile(url)) {
                    if (unzipFile()) {
                        if (data_type_ == settings::DataType::TRADES) {
                            readFuturesTradeFile(url, symbol);
                        } else if (data_type_ == settings::DataType::OHLCV) {
                            readCandleFile(url, symbol);
                        } else {
                            deleteFile();
                            break;
                        }
                    }
                    deleteFile();
                }
                break;
            }
        }
        context_->producerDone.store(true);
    }

    bool FileDownloader::downloadFile(const std::string &url) const {
        const auto file_path = tmp_dir_file_ / "data.zip";
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file " << file_path << std::endl;
            return false;
        }

        auto write_callback = [&](const std::string_view data, intptr_t) {
            file.write(data.data(), data.size());
            return true;
        };

        if (const cpr::Response r = cpr::Get(cpr::Url{url}, cpr::WriteCallback{write_callback}); r.error) {
            std::cerr << "Error downloading file from " << url << ": " << r.error.message << std::endl;
            return false;
        }
        std::cout << "Downloaded file from " << url << " to " << file_path << std::endl;
        return true;
    }

     bool FileDownloader::unzipFile() const {
        auto zip_path = tmp_dir_file_ / "data.zip";

        try {
            elz::extractZip(tmp_dir_file_);
            return true;
        }
        catch (const elz::zip_exception& e) {
            std::cerr << "Failed to extract zip file from " << tmp_dir_file_ << std::endl;
            return false;
        }
    }

    void FileDownloader::readFuturesTradeFile(const std::string& url, const std::string& symbol) const {
        // get file name - final part of url - remove .zip and replace with .csv
        const auto file_name_start = url.find_last_of('/') + 1;
        const auto file_name_end = url.find(".zip");
        const auto file_name = url.substr(file_name_start, file_name_end - file_name_start) + ".csv";
        const auto file_path = tmp_dir_file_ / "data" / file_name;

        std::ifstream file(file_path);

        if (!file.is_open()) {
            std::cerr << "Failed to open file " << file_path << std::endl;
            return;
        }
        // skip the header line
        std::string line;
        std::getline(file, line);
        // structure
        // 0 - id, 1 - price, 2 - qty, 3 - quoteQty, 4 - time, 5 - isBuyerMaker
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            try {
                std::string token;
                models::Trade trade;
                // id
                std::getline(ss, token, ',');
                trade.id = std::stoll(token);
                // price
                std::getline(ss, token, ',');
                trade.price = std::stod(token);
                // qty
                std::getline(ss, token, ',');
                trade.qty = std::stod(token);
                // quoteQty
                std::getline(ss, token, ',');
                trade.quote_qty = std::stod(token);
                // time
                std::getline(ss, token, ',');
                trade.time = std::stoll(token);
                // isBuyerMaker == to Side
                std::getline(ss, token, ',');
                const bool is_buyer_maker = (token == "true");
                trade.side = models::getTradeSide(is_buyer_maker);
                trade.symbol =  symbol;
                trade.product_type = product_type_;

                models::DataEvent event;
                event.futures_trade = trade;
                queue_.enqueue(event);
            } catch (const std::exception &e) {
                std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
            }
        }
        file.close();
    }

    void FileDownloader::readCandleFile(const std::string& url, const std::string& symbol) const {
        // get file name - final part of url - remove .zip and replace with .csv
        const auto file_name_start = url.find_last_of('/') + 1;
        const auto file_name_end = url.find(".zip");
        const auto file_name = url.substr(file_name_start, file_name_end - file_name_start) + ".csv";
        const auto file_path = tmp_dir_file_ / "data" / file_name;

        std::ifstream file(file_path);

        if (!file.is_open()) {
            std::cerr << "Failed to open file " << file_path << std::endl;
            return;
        }
        // skip the header line
        std::string line;
        std::getline(file, line);
        // structure
        // 0 - open_time, 1 - open, 2 - high, 3 - low, 4 - close, 5 - volume, 6 - close_time
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            try {
                std::string token;
                models::Candle candle;
                // open_time
                std::getline(ss, token, ',');
                candle.open_time = std::stoll(token);
                // open
                std::getline(ss, token, ',');
                candle.open = std::stod(token);
                // high
                std::getline(ss, token, ',');
                candle.high = std::stod(token);
                // low
                std::getline(ss, token, ',');
                candle.low = std::stod(token);
                // close
                std::getline(ss, token, ',');
                candle.close = std::stod(token);
                // volume
                std::getline(ss, token, ',');
                candle.volume = std::stod(token);
                // close_time
                std::getline(ss, token, ',');
                candle.close_time = std::stoll(token);

                candle.symbol = symbol;
                candle.product_type = product_type_;
                candle.frequency = settings::getCandleFrequency(settings::getCandleFrequencyName(download_type_ == settings::MONTHLY ? settings::ONE_MONTH : settings::ONE_DAY));

                models::DataEvent event;
                event.candle = candle;
                queue_.enqueue(event);
            } catch (const std::exception &e) {
                std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
            }
        }
    }

    void FileDownloader::deleteFile() const {
        try {
            const auto zipPath = tmp_dir_file_ / "data.zip";
            const auto decompressedPath = tmp_dir_file_ / "data";
            if (std::filesystem::exists(zipPath)) {
                std::filesystem::remove(zipPath);
            }
            if (std::filesystem::exists(decompressedPath)) {
                std::filesystem::remove_all(decompressedPath);
            }
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

    std::vector<std::string> FileDownloader::createUrls(const std::string &symbol, const std::string &start_date, const std::string &end_date) const {
        std::vector<std::string> urls;
        try {
            const auto start_date_ymd = parseDateString(start_date);
            const auto end_date_ymd = parseDateString(end_date);

            auto current_date_sys = std::chrono::sys_days(start_date_ymd);
            const auto end_date_sys = std::chrono::sys_days(end_date_ymd);


            const std::string base_url = models::getFuturesUrl(symbol, settings::getDownloadTypeName(download_type_), settings::getDataTypeName(data_type_));

            while (current_date_sys < end_date_sys) {
                std::chrono::year_month_day current_ymd(current_date_sys);
                std::string formatted_date;
                std::string formatted_url;

                if (download_type_ == settings::MONTHLY) {
                    formatted_date = std::format("%Y-%m", current_ymd);
                    formatted_url = base_url + models::getFileName(symbol, formatted_date, settings::getDataTypeName(data_type_));

                    auto year = current_ymd.year();
                    auto month =  current_ymd.month();
                    if (month == std::chrono::December) {
                        year += std::chrono::years(1);
                        month = std::chrono::January;
                    } else {
                        month += std::chrono::months(1);
                    }

                    auto next_month = year / month / std::chrono::day{1};
                    current_date_sys = std::chrono::sys_days(next_month);

                } else if (download_type_ == settings::DAILY) {
                    formatted_date = std::format("%Y-%m-%d", current_ymd);
                    formatted_url = base_url + models::getFileName(symbol, formatted_date, settings::getDataTypeName(data_type_));
                    current_date_sys += std::chrono::days(1);
                }
                urls.push_back(formatted_url);
            }

        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            return {};
        }
        return urls;
    }

    std::chrono::year_month_day FileDownloader::parseDateString(const std::string &dateString) {
        std::stringstream ss(dateString);
        int year, month, day;

        if (char dash; !(ss >> year >> dash >> month >> dash >> day)) {
            throw std::invalid_argument("Invalid date format. Expected YYYY-MM-DD.");
        }
        return std::chrono::year_month_day(
            std::chrono::year(year),
            std::chrono::month(month),
            std::chrono::day(day)
        );
    }
}
