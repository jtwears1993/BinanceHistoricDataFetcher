//
// Created by jtwears on 10/26/25.
//

#include "common/network/socket/multicast_server.h"
#include "common/network/socket/utils.h"
#include "common/macros.h"

namespace common::network::sockets {

    auto MulticastServer::join(const std::string &ip) const -> bool {
        return sockets::join(socket_fd_, ip);
    }

    auto MulticastServer::send_and_receive() noexcept -> bool {
        // Read data and dispatch callbacks if data is available - non-blocking.
        const ssize_t n_rcv = recv(socket_fd_,
            recv_buffer_.data() + recv_next_valid_index_,
            MulticastBufferSize - recv_next_valid_index_,
            MSG_DONTWAIT);
        if (n_rcv > 0) {
            recv_next_valid_index_ += n_rcv;
            recv_callback_(this);
        }

        // Publish market data in the send buffer to the multicast stream.
        if (send_next_valid_index_ > 0) {
            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(config_.port);
            addr.sin_addr.s_addr = inet_addr(config_.ip.c_str());
            unsigned int addrlen = sizeof(addr);
            ::sendto(socket_fd_,
                   send_buffer_.data(),
                   send_next_valid_index_,
                   0,
                   reinterpret_cast<sockaddr *>(&addr),
                   addrlen);
        }
        send_next_valid_index_ = 0;
        return (n_rcv > 0);
    }

    auto MulticastServer::send(const void *data, const size_t len) noexcept -> void {
        memcpy(send_buffer_.data() + send_next_valid_index_, data, len);
        send_next_valid_index_ += len;
        ASSERT(send_next_valid_index_ < MulticastBufferSize,
            "Mcast socket buffer filled up and send_and_recv() not called."
        );
    }

}