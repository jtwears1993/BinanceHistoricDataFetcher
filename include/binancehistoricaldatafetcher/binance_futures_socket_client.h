//
// Created by jtwears on 10/4/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H

#include <thread>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/websocketpp/client.hpp>
#include <concurrentqueue/concurrentqueue.h>
#include <nlohmann/json.hpp>

#include "binancehistoricaldatafetcher/binance_market_data_models.h"

namespace downloader {


    typedef websocketpp::client<websocketpp::config::asio_client> client;
    typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;

    using json = nlohmann::json;


    template<typename SocketEvent>
    class BinanceFuturesSocketClient {
    protected:
        std::string uri_;
        client c_;
        websocketpp::connection_hdl hdl_;
        std::unique_ptr<std::thread> worker_thread_;
        models::BinanceFuturesOnOpenSocketMessage socket_open_msg_;
        std::unordered_map<std::string, moodycamel::ConcurrentQueue<SocketEvent>*> event_queues_;

    public:
        explicit BinanceFuturesSocketClient(std::string uri,
            const models::BinanceFuturesOnOpenSocketMessage &open_msg,
            std::unordered_map<std::string, moodycamel::ConcurrentQueue<SocketEvent>*> events_queue) :
        uri_(std::move(uri)),
        socket_open_msg_(open_msg),
        event_queues_(events_queue) {}

        virtual ~BinanceFuturesSocketClient() = default;

        void on_open(websocketpp::connection_hdl hdl, client* c) {
            std::cout << "BinanceFuturesSocketClient::on_open" << std::endl;
            std::cout << "Connection established." << std::endl;

            json sub_request;
            sub_request["method"] = "SUBSCRIBE";
            sub_request["params"] = socket_open_msg_.params;
            sub_request["id"] = socket_open_msg_.id;

            const std::string request_str = sub_request.dump();
            std::cout << "Sending subscription request: " << request_str << std::endl;
            websocketpp::lib::error_code ec;
            c->send(hdl, request_str, websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cout << "Error sending subscription request: " << ec.message() << std::endl;
            } else {
                std::cout << "Subscribed successfully to: " << request_str << std::endl;
            }
        }

        virtual void on_message(websocketpp::connection_hdl, client::message_ptr msg) = 0;

        int start() {
            try {
                // Set logging to be pretty verbose (everything except message payloads)
                c_.set_access_channels(websocketpp::log::alevel::all);
                c_.clear_access_channels(websocketpp::log::alevel::frame_payload);

                // Initialize ASIO
                c_.init_asio();

                // Register our handlers
                c_.set_open_handler(bind(&BinanceFuturesSocketClient::on_open, this, _1, &c_));
                c_.set_message_handler(bind(&BinanceFuturesSocketClient::on_message, this, _1, _2));
                c_.set_error_channels(websocketpp::log::elevel::all); // enabling detailed error logging
                websocketpp::lib::error_code ec;
                const client::connection_ptr con = c_.get_connection(uri_, ec);
                if (ec) {
                    std::cout << "could not create connection because: " << ec.message() << std::endl;
                    return -1;
                }

                // Note that connect here only requests a connection. No network messages are
                // exchanged until the event loop starts running in the next line.
                hdl_ = con->get_handle();
                c_.connect(con);

                // Start the ASIO io_service run loop
                // this will cause a single connection to be made to the server. c.run()
                // will exit when this connection is closed.
                // Run the ASIO io_service on a background thread
                // Closed in main thread by calling stop, which joins the thread and closes the connection
                worker_thread_ = std::make_unique<std::thread>([this]() { c_.run(); });
            } catch (const std::exception &e) {
                std::cout << "Exception: " << e.what() << std::endl;
            } catch (websocketpp::lib::error_code e) {
                std::cout << "Error Code: " << e.message() << std::endl;
            } catch (...) {
                std::cout << "Other exception" << std::endl;
            }
            return 0;
        }

        void stop() {
            c_.stop();

            if (worker_thread_ && worker_thread_->joinable()) {
                worker_thread_->join();
            }
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H