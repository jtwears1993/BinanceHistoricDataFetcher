#include <string>
#include <iostream>
#include <memory>

#include "../../include/libs/CLI11.hpp"

#include "binancehistoricaldatafetcher/constants.h"
#include "binancehistoricaldatafetcher/HistoricalDataProcessor.h"
#include "binancehistoricaldatafetcher/file_downloader.h"
#include "common/io/questdb_writer.h"
#include "common/models/enums.h"
#include "common/sync/producer_consumer.h"
#include "common/models/common_data_models.h"

// TODO - DEBUG - THIS WILL NOT RUN; LOGIC IS BROKEN - JUST FOR REFERENCE

using namespace common::models;
using namespace common::models::enums;

auto build_exchange_info_map() {
    std::unordered_map<std::string, ExchangeInfo> exchange_info;
    ExchangeInfo info{};
    info.tick_size = 3;
    info.step_size = 4;
    exchange_info["btcusdt"] = info;
    return exchange_info;
}

int main(const int argc, char** argv) {
    CLI::App app{APP_NAME};

    app.set_version_flag("--version", APP_VERSION);

    app.add_flag("--product", "Product type: futures, options, spot")
        ->required()
        ->check(CLI::IsMember({"futures", "options", "spot"}));
    app.add_flag("--downloadType", "Download type: monthly, daily")
        ->required()
        ->check(CLI::IsMember({"monthly", "daily"}));
    app.add_flag("--outputType", "Output type: parquet, questdb")
        ->required()
        ->check(CLI::IsMember({"parquet", "questdb"}));
    app.add_option("--start", "Start date in YYYY-MM-DD format")
        ->required();
    app.add_option("--end", "End date in YYYY-MM-DD format")
        ->required();
    app.add_option("--symbols", "Comma-separated list of symbols")
        ->required();
    app.add_option("--dbURL", "Database URL for QuestDB output");

    try {
        app.parse(argc, argv);
        auto settings = binance::settings::Settings();
        auto start = app.get_option("--start")->as<std::string>();
        auto end = app.get_option("--end")->as<std::string>();
        auto downloadType = getDownloadType(app.get_option("--downloadType")->as<std::string>());
        auto product = getProduct(app.get_option("--product")->as<std::string>());
        const auto outputType = getOutputType(app.get_option("--outputType")->as<std::string>());
        auto symbols = app.get_option("--symbols")->as<std::vector<std::string>>();

        if (outputType == QUESTDB) {
            settings.dbUrl = app.get_option("--dbURL")->as<std::string>();
        } else {
            std::cerr << "Error: parquet support still under development" << std::endl;
            return EXIT_FAILURE;
        }

        settings.downloadType = downloadType;
        settings.outputType = outputType;
        settings.startDate = start;
        settings.endDate = end;
        settings.symbols = symbols;
        settings.product = product;
        settings.batchSize = BATCH_SIZE;

        auto context = std::make_shared<Context>();

        auto buffer = moodycamel::ConcurrentQueue<DataEvent>(BUFFER_SIZE);

        auto downloader = std::make_unique<downloader::FileDownloader>(
            settings.dataType,
            settings.product,
            settings.downloadType,
            buffer,
            context
        );
        auto dbURI = settings.dbUrl.value();
        auto exchange_info = std::make_shared<std::unordered_map<std::string, ExchangeInfo>>();
        auto writer = std::make_unique<writer::QuestDBWriter>(
            buffer,
            dbURI,
            context,
            exchange_info,
            settings.batchSize,
            FLUSH_INTERVAL_MS,
            settings.dataType
        );

        auto processor = binance::processor::HistoricalDataProcessor(context, std::move(writer), std::move(downloader), std::make_unique<Settings>(settings));
        processor.process();

    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
