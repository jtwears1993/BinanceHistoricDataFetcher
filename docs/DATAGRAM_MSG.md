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

```python
# Final, robust standard configuration for Linux receiver: Global binding + INADDR_ANY Join.

import socket
import struct
import sys
import time
import json

# --- Configuration ---
MULTICAST_GROUP = '233.252.14.1'
MULTICAST_PORT = 20000
# CRITICAL: Standard receiver bind: Listen on all interfaces.
BIND_ADDRESS = '0.0.0.0'
HEADER_SIZE_IN_BYTES = 7
FOOTER_SIZE_IN_BYTES = 3

# We no longer explicitly refer to 127.0.0.1 here for the join structure,
# relying on INADDR_ANY (0) which is sometimes necessary to fix this issue.

def setup_multicast_socket():
    """Sets up the multicast UDP socket for receiving market data."""
    try:
        # Create a UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

        # Allow reuse of the address/port (essential for multicast)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Bind the socket to the general interface IP and the port
        print(f"INFO: Binding socket to {BIND_ADDRESS}:{MULTICAST_PORT}")
        sock.bind((BIND_ADDRESS, MULTICAST_PORT))

        # --- Multicast Group Join (The Ultimate Tweak) ---
        loopback_ip = socket.inet_aton('127.0.0.1')
        mreq = struct.pack('4s4s', socket.inet_aton(MULTICAST_GROUP), loopback_ip)
        # Set the socket option to join the group
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

        print(f"SUCCESS: Joined multicast group {MULTICAST_GROUP} on all interfaces (using INADDR_ANY).")
        return sock

    except OSError as e:
        print(f"Error setting up socket: {e}")
        print("HINT: If you see 'Address already in use', ensure SO_REUSEADDR is set on the C++ sender too!")
        sys.exit(1)
    except Exception as e:
        print(f"An unexpected error occurred during setup: {e}")
        sys.exit(1)

def calculate_diffs(
    last_bids: list[list[str, int, str, int]],
    current_snapshot: list[list[str, int, str, int]],
) -> list[dict]:
    deltas = []

    # Convert to dict by price for easier diffing
    old_map = {bid[1]: bid[3] for bid in last_bids}
    new_map = {bid[1]: bid[3] for bid in current_snapshot}

    all_prices = sorted(set(old_map.keys()) | set(new_map.keys()), reverse=True)

    for level, price in enumerate(all_prices):
        old_qty = old_map.get(price)
        new_qty = new_map.get(price)

        if old_qty is None:
            # New level added
            deltas.append({
                "action": "add",
                "price": price,
                "new_qty": new_qty,
                "level": level,
            })
        elif new_qty is None:
            # Level removed
            deltas.append({
                "action": "delete",
                "price": price,
                "old_qty": old_qty,
                "level": level,
            })
        elif old_qty != new_qty:
            # Quantity changed
            deltas.append({
                "action": "update",
                "price": price,
                "old_qty": old_qty,
                "new_qty": new_qty,
                "level": level,
            })

    return deltas



def main():
    sock = setup_multicast_socket()

    print("Listening for market data...")
    print("-" * 30)
    last_message = {
        "bids": [],
        "asks": []
    }
    try:
        while True:
            # Receive data (max 4096 bytes)
            data, address = sock.recvfrom(4096)
            # Timestamp the reception
            current_time_ms = int(time.time() * 1000)
            message = data.decode('utf-8', errors='ignore')
            message_start = message.find("{")
            json_string = message[message_start : ]
            parsed_msg = json.loads(json_string)

            # skip first iteration
            if (len(last_message["bids"]) > 0) and (len(last_message["asks"]) > 0):
                diffs_bids = calculate_diffs(last_message["bids"], parsed_msg["payload"]["snapshot"]["bids"])
                print(f"[{current_time_ms}] RECV from {address}. DIFFS BIDS: {diffs_bids}")
                diffs_asks = calculate_diffs(last_message["asks"], parsed_msg["payload"]["snapshot"]["asks"])
                print(f"[{current_time_ms}] RECV from {address}. DIFFS ASKS: {diffs_asks}")
            last_message["asks"] = parsed_msg["payload"]["snapshot"]["asks"]
            last_message["bids"] = parsed_msg["payload"]["snapshot"]["bids"]
    except KeyboardInterrupt:
        print("\nReceiver stopped by user.")
    except Exception as e:
        print(f"Error during reception loop: {e}")
    finally:
        sock.close()
        print("Socket closed.")


if __name__ == '__main__':
    main()

```

-----

## 3\. JSON Payload Structure

The raw JSON payload contains the serialized `models::DataEvent`. This object uses optional fields, meaning the received JSON object will contain **only the data types that were active** in that specific event.

| JSON Key | Corresponds to C++ Type | Description |
| :--- | :--- | :--- |
| **`snapshot`** | `OrderbookSnapshot` | A full or partial snapshot of the order book. |
| **`futures_trade`** | `Trade` | A single execution (trade) event. |
| **`candle`** | `Candle` | A single aggregated candle (bar) event. |