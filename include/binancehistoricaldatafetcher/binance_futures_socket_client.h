//
// Created by jtwears on 10/4/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H
#define BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/websocketpp/client.hpp>
#include <thread>
#include <string>
#include <utility>
#include <unordered_map>
#include <memory>
#include <atomic>

#include <concurrentqueue/concurrentqueue.h>
#include <nlohmann/json.hpp>

#include "binancehistoricaldatafetcher/binance_market_data_models.h"

namespace downloader {


    typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
    typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;

    using json = nlohmann::json;

    inline std::string get_host_from_uri(const std::string& uri) {
        const std::string prefix = "wss://";
        size_t start = uri.find(prefix);
        if (start == std::string::npos) {
            return "";
        }
        start += prefix.length();
        size_t end = uri.find('/', start);
        if (end == std::string::npos) {
            end = uri.length();
        }
        return uri.substr(start, end - start);
    }

    template<typename SocketEvent>
    class BinanceFuturesSocketClient {
    protected:
        std::string uri_;
        client c_;
        websocketpp::connection_hdl hdl_;
        std::unique_ptr<std::thread> worker_thread_;
        models::BinanceFuturesOnOpenSocketMessage socket_open_msg_;
        std::unordered_map<std::string, std::shared_ptr<moodycamel::ConcurrentQueue<SocketEvent>>> event_queues_;
        std::atomic<bool> should_reconnect_{true};
        std::atomic<bool> is_reconnecting_{false};

    public:
        explicit BinanceFuturesSocketClient(std::string uri,
            const models::BinanceFuturesOnOpenSocketMessage &open_msg,
            std::unordered_map<std::string, std::shared_ptr<moodycamel::ConcurrentQueue<SocketEvent>>> events_queue) :
        uri_(std::move(uri)),
        socket_open_msg_(open_msg),
        event_queues_(events_queue) {}

        virtual ~BinanceFuturesSocketClient() = default;

        void on_open(websocketpp::connection_hdl hdl, client* c) {
            std::cout << "INFO::BinanceFuturesSocketClient::on_open \n";
            std::cout << "INFO::BinanceFuturesSocketClient::on_open Connection established. Sending subscription request...\n";

            json sub_request;
            sub_request["method"] = socket_open_msg_.method;
            sub_request["params"] = socket_open_msg_.params;
            sub_request["id"] = socket_open_msg_.id;
            const std::string request_str = sub_request.dump();
            websocketpp::lib::error_code ec;
            c->send(hdl, request_str, websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cout << "ERROR::BinanceFuturesSocketClient::on_open Error sending subscription request: " << ec.message() << std::endl;
            } else {
                std::cout << "INFO::Subscribed successfully to: " << request_str << std::endl;
            }
        }

        void on_close(websocketpp::connection_hdl hdl) {
            std::cout << "INFO::BinanceFuturesSocketClient::on_close BinanceFuturesSocketClient::on_close handling connection close \n";
            if (!should_reconnect_) {
                std::cout << "INFO::BinanceFuturesSocketClient::on_close BinanceFuturesSocketClient::on_close Not reconnecting as should_reconnect_ is false\n";
                return;
            }

            if (is_reconnecting_.exchange(true)) {
                std::cout << "INFO::BinanceFuturesSocketClient::on_close Already reconnecting, skipping...\n";
                return;
            }

            std::cout << "INFO::BinanceFuturesSocketClient::on_close Connection closed. Attempting to reconnect...\n";
            stop();
            if (const auto res = start(); res != EXIT_SUCCESS) {
                std::cerr << "ERROR::BinanceFuturesSocketClient::on_close Failed to reconnect after connection closed\n";
            }
            std::cout << "INFO::BinanceFuturesSocketClient::on_close connection re-established\n";
            is_reconnecting_ = false;
        }

        context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl) {
            auto ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
            try {
                ctx->set_options(boost::asio::ssl::context::default_workarounds |
                                 boost::asio::ssl::context::no_sslv2 |
                                 boost::asio::ssl::context::no_sslv3 |
                                 boost::asio::ssl::context::single_dh_use);
                ctx->set_verify_mode(boost::asio::ssl::verify_none);
            } catch (std::exception& e) {
                std::cout << "ERROR::BinanceFuturesSocketClient::on_tls_init TLS Initialization Error: " << e.what() << std::endl;
            }
            return ctx;
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
                c_.set_tls_init_handler(bind(&BinanceFuturesSocketClient::on_tls_init, this, get_host_from_uri(uri_).c_str(), _1));
                c_.set_error_channels(websocketpp::log::elevel::all); // enabling detailed error logging
                c_.set_close_handler(bind(&BinanceFuturesSocketClient::on_close, this, _1));
                websocketpp::lib::error_code ec;
                const client::connection_ptr con = c_.get_connection(uri_, ec);
                if (ec) {
                    std::cerr << "FATAL::BinanceFuturesSocketClient::start could not create connection because: " << ec.message() << std::endl;
                    return EXIT_FAILURE;
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
                std::cerr << "ERROR::BinanceFuturesSocketClient::start Exception: " << e.what() << std::endl;
                return EXIT_FAILURE;
            } catch (websocketpp::lib::error_code e) {
                std::cerr << "ERROR::BinanceFuturesSocketClient::start Error Code: " << e.message() << std::endl;
                return EXIT_FAILURE;
            } catch (...) {
                std::cerr << "ERROR::BinanceFuturesSocketClient::start Other exception" << std::endl;
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        void stop() {
            c_.stop();
            should_reconnect_ = false;
            if (worker_thread_ && worker_thread_->joinable()) {
                worker_thread_->join();
            }
            std::cout << "INFO::BinanceFuturesSocketClient::stop Connection stopped and worker thread joined. Exiting Successfully." << std::endl;
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_BINANCE_FUTURES_SOCKET_CLIENT_H