# 📦 Binance Market Data Datagram Specification

This document describes the structure of the UDP multicast datagrams sent by the `MarketDataPublisher` and provides guidance on how client applications should read and parse the data.

-----

## 1\. Datagram Framing Protocol

The entire message is sent as a single UDP datagram, which is a contiguous block of bytes. The message uses a simple **three-part framing protocol** to ensure the variable-length JSON payload can be reliably read by the receiver.

The structure is: **Sequence ID** | **Payload Length** | **JSON Payload**

| Section | C++ Type | Size (Bytes) | Description |
| :--- | :--- | :--- | :--- |
| **1. Sequence ID** | `std::atomic<size_t>` | $\approx 8$ | A global, monotonic counter for the stream. Used to detect missing or out-of-order packets. |
| **2. Payload Length (N)** | `uint32_t` | **4** | The size, in bytes, of the JSON payload that immediately follows. **This is the key to parsing.** |
| **3. JSON Payload** | `std::string::data()` | $N$ (Variable) | The serialized market data (the `DataEvent`). |

**Total Header Size:** $\approx 12$ bytes (8 bytes for `size_t` + 4 bytes for `uint32_t`).

-----

## 2\. Reading and Unpacking Protocol (Pseudocode)

The client application must implement a routine to correctly slice the raw datagram into its three logical parts. This process requires using a fixed-size header structure to unpack the initial binary data.

Assuming **Network Byte Order (Big-Endian)**:

```pseudocode
FUNCTION process_datagram(raw_datagram):
    HEADER_SIZE = 12  // 8 bytes for Seq ID + 4 bytes for Length
    HEADER_FORMAT = "!QI" // Big-Endian, Unsigned Long Long (size_t), Unsigned Int (uint32_t)

    // --- Step 1: Unpack Headers (First 12 bytes) ---
    raw_header = raw_datagram[0 : HEADER_SIZE]
    (sequence_id, payload_length) = UNPACK(HEADER_FORMAT, raw_header)

    // --- Step 2: Extract JSON Payload ---
    json_start_index = HEADER_SIZE
    json_end_index = json_start_index + payload_length

    // Extract the exact number of bytes specified by payload_length
    raw_json_payload = raw_datagram[json_start_index : json_end_index]

    // --- Step 3: Decode and Parse ---
    IF LENGTH(raw_json_payload) == payload_length:
        json_string = DECODE_UTF8(raw_json_payload)
        data_event = PARSE_JSON(json_string)
        
        // --- Step 4: Process ---
        PROCESS_DATA_EVENT(sequence_id, data_event)
    ELSE:
        LOG_ERROR("Datagram truncated: Payload size mismatch.")

FUNCTION PROCESS_DATA_EVENT(sequence_id, data_event):
    IF "snapshot" IN data_event:
        HANDLE_SNAPSHOT(data_event["snapshot"])
    ELSE IF "futures_trade" IN data_event:
        HANDLE_TRADE(data_event["futures_trade"])
    ELSE IF "candle" IN data_event:
        HANDLE_CANDLE(data_event["candle"])
```

-----

## 3\. JSON Payload Structure

The raw JSON payload contains the serialized `models::DataEvent`. This object uses optional fields, meaning the received JSON object will contain **only the data types that were active** in that specific event.

| JSON Key | Corresponds to C++ Type | Description |
| :--- | :--- | :--- |
| **`snapshot`** | `OrderbookSnapshot` | A full or partial snapshot of the order book. |
| **`futures_trade`** | `Trade` | A single execution (trade) event. |
| **`candle`** | `Candle` | A single aggregated candle (bar) event. |