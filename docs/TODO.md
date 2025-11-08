# TODO
## COMMON:

- Add reference data service for symbol to int and replace string symbol with int symbol throughout the codebase
- specfic type aliases for: tick_size, step_size, price and quantity (e.g., using type Price = int_32_t)
- specific type alias for symbol string (e.g., using type Symbol = std::string)
- look at implementing multi-symbolbook as an array or vector of orderbooks instead of map of orderbooks and use an enum for symbol index position
- Generic get_symbols function for parsing cli arguments and config files
- Add unix domain socket support for server-client communication
- Create a zero copy and alloc logger for high frequency logging
- Add unit tests for order book builder and order book 
- Add benchmarks for order book builder, multi-symbol order book and server-client communication
- Add Aeron support for server-client communication
- Rename Repo CryptoMarketData
- Once an extra exchange is integrated, create an orderbook aggregator that can aggregate orderbooks from multiple exchanges into a single orderbook

## LIVE DATA:

- Clean up client-server communication code after debugging
- Add examples directory with python client example (mainly for testing and my own memory)
- Support for trades and candles streaming
- Add more exchanges (Binance Spot, Gemini, Coinbase Pro, etc)


## ARCHIVER:

- Support trades and candle archiving 
- Add more exchanges (Binance Spot, Gemini, Coinbase Pro, etc)

## CLI:

- Debug downloading, parsing and archiving process
- Add more exchanges (Binance Spot, Gemini, Coinbase Pro, etc)
- Create shel script for easy deployment and testing
- Create shell script and schedule cron job for automatic archiving of trades 

## QUEST DB

- Identify cost-effective server solution: Velia or OVH likely options
- Configure and deploy QuestDB server