cmake_minimum_required(VERSION 4.0)
project(BinanceHistoricDataFetcher)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        c_questdb_client_proj
        GIT_REPOSITORY https://github.com/questdb/c-questdb-client.git
        GIT_TAG 5.0.0)

FetchContent_MakeAvailable(c_questdb_client_proj)

set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system curl")

FetchContent_Declare(
        cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.12.0
)

FetchContent_MakeAvailable(cpr)

find_package(Boost REQUIRED COMPONENTS system filesystem)

add_subdirectory(include/libs/11Zip)

add_executable(BinanceHistoricDataFetcher app/main.cpp
        include/binancehistoricaldatafetcher/questdb_writer.h
        include/binancehistoricaldatafetcher/file_downloader.h
        src/file_downloader.cpp
        src/questdb_writer.cpp
        include/binancehistoricaldatafetcher/settings.h
        include/libs/CLI11.hpp
        include/binancehistoricaldatafetcher/constants.h
        src/settings.cpp
        include/binancehistoricaldatafetcher/processor.h
        src/processor.cpp
        include/binancehistoricaldatafetcher/writer.h
        src/writer.cpp
)
target_include_directories(BinanceHistoricDataFetcher PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(
        BinanceHistoricDataFetcher
        questdb_client
        cpr::cpr
        elzip
)
