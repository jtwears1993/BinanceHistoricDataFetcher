//
// Created by jtwears on 10/26/25.
//
#pragma once

#include <functional>
#include <vector>
#include <string>
#include <unistd.h>

#include "utils.h"

namespace common::network::sockets {
    class MulticastServer {
        int socket_fd_{-1};
        const SocketConfig config_;
        std::vector<char> recv_buffer_;
        size_t recv_next_valid_index_{0};
        std::vector<char> send_buffer_;
        size_t send_next_valid_index_{0};
        std::string timestamp_str_;
        /// Function wrapper for the method to call when data is read.
        std::function<void(MulticastServer *s)> recv_callback_ = nullptr;

    public:

        explicit MulticastServer(const SocketConfig &config) : config_(config) {
            recv_buffer_.resize(MulticastBufferSize);
            send_buffer_.resize(MulticastBufferSize);
        }

        ~MulticastServer() {
            stop();
        }

        auto start() -> int {
            socket_fd_ = create_socket(config_);
            return socket_fd_;
        }

        auto stop() -> void {
            if (socket_fd_ != -1) {
                close(socket_fd_);
                socket_fd_ = -1;
            }
        }

        auto join(const std::string &ip) const -> bool;

        auto send_and_receive() noexcept -> bool;

        auto send(const void *data, size_t len) noexcept -> void;

        auto set_recv_callback(const std::function<void(MulticastServer *s)> &callback) -> void {
            recv_callback_ = callback;
        }
    };
}
