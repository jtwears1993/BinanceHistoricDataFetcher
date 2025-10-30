# TODO
## COMMON:

- Generic get_symbols function for parsing cli arguments and config files
- Add unix domain socket support for server-client communication
- Create a zero copy and alloc logger for high frequency logging
- Add unit tests for order book builder and order book 
- Add benchmarks for order book builder, multi-symbol order book and server-client communication
- Add Aeron support for server-client communication
- Re-organize project structure into multiple packages/modules:
    1. Binance Logic in own package
    2. Common data models extracted out into common package
    3. QuestDB extracted into common  
    4. IWriter interface extracted into common package
    5. Orderbook and MultiSymbolOrderbook extracted into common
- Rename Repo CryptoMarketData
- Once an extra exchange is integrated, create an orderbook aggregator that can aggregate orderbooks from multiple exchanges into a single orderbook

## LIVE DATA:

- Fix send buffer bug; when full keeps re-sending old data. should be:
   ```
    while total_sent < len(data):
        nbytes = sock.send(data[total_sent:])
        if nbytes == 0:
            raise RuntimeError("socket connection broken")
        total_sent += nbytes
        if total_sent == len(data):
            break
   ```
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