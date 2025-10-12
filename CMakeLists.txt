cmake_minimum_required(VERSION 4.0)
project(BinanceHistoricDataFetcher CXX)

set(CMAKE_CXX_STANDARD 20)

# --- External Dependencies ---

include(FetchContent)

# QuestDB Client
FetchContent_Declare(
        c_questdb_client_proj
        GIT_REPOSITORY https://github.com/questdb/c-questdb-client.git
        GIT_TAG 5.0.0
)
FetchContent_MakeAvailable(c_questdb_client_proj)

# CPR (requires system curl to be ON)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system curl")
FetchContent_Declare(
        cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.12.0
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)


# Elzip (subdirectory)
add_subdirectory(include/libs/11Zip)

# Websocket++ (subdirectory)
#add_subdirectory(include/libs/websocketpp/websocketpp)

# --- 1. Define Shared Logic Library ---

# Collect all common source files into a library so both executables can link to them.
# NOTE: Headers should generally NOT be listed in add_library or add_executable
add_library(
        binance_shared_logic STATIC
        src/file_downloader.cpp
        src/questdb_writer.cpp
        src/settings.cpp
        src/processor.cpp
        src/writer.cpp
        src/binance_futures_orderbook.cpp
)

# Set common include directories for the shared logic
target_include_directories(binance_shared_logic
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        # CLI11 is header-only, but adding its directory for includes
        include/libs
        $<TARGET_PROPERTY:cpr::cpr,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:questdb_client,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:elzip,INTERFACE_INCLUDE_DIRECTORIES>
        ${PROJECT_SOURCE_DIR}/include/libs/websocketpp
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

# --- 2. Define the CLI Executable ---

add_executable(
        binance_cli_tool      # Unique name for the CLI executable
        app/cli/main.cpp      # Single, unique entry point
)

# Link the CLI tool to the shared logic and dependencies
target_link_libraries(
        binance_cli_tool
        binance_shared_logic
        questdb_client
        cpr::cpr
        elzip
        nlohmann_json::nlohmann_json
)

# --- 3. Define the Continuous Application Executable ---

add_executable(
        binance_archiver_app   # Unique name for the server/archiver executable
        app/archiver/main.cpp
)

# Link the continuous app to the shared logic and dependencies
target_link_libraries(
        binance_archiver_app
        binance_shared_logic
        questdb_client
        cpr::cpr
        elzip
        nlohmann_json::nlohmann_json
)