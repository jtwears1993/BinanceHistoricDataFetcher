//
// Created by jtwears on 10/26/25.
//

#pragma once

#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>

namespace common::network::sockets {

    constexpr auto MulticastLocalHostIP = "127.0.0.1";

    enum class SocketType {
        UDP,
        TCP,
        UNIX
    };

    struct SocketConfig {
        std::string ip;
        std::string interface;
        int port;
        SocketType type;
        bool is_listening;
        bool need_so_timestamp;

        auto to_string() const -> std::string {
            return "SocketConfig{ip: " + ip +
                   ", interface: " + interface +
                   ", port: " + std::to_string(port) +
                   ", type: " + (type == SocketType::UDP ? "UDP" : type == SocketType::TCP ? "TCP" : "UNIX") +
                   ", is_listening: " + (is_listening ? "true" : "false") +
                   ", need_so_timestamp: " + (need_so_timestamp ? "true" : "false") +
                   "}";
        }
    };

    constexpr auto MaxTCPBacklog = 1024;
    constexpr auto MulticastBufferSize = 64 * 1024 * 1024; // 64 MB

    auto inline get_interface_ip(const std::string& interface_name) -> std::string {
        char ip_buffer[NI_MAXHOST] = {0};
        ifaddrs *ifaddr = nullptr;

        if (getifaddrs(&ifaddr) != -1) {
            for (const ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && interface_name == ifa->ifa_name) {
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), ip_buffer, sizeof(ip_buffer), nullptr, 0, NI_NUMERICHOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        return std::string(ip_buffer);
    }

    auto inline set_non_blocking(const int socket_fd) -> bool {
        const auto flags = fcntl(socket_fd, F_GETFL, 0);
        if (flags & O_NONBLOCK)
            return true;
        return (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    auto inline disable_nagle_algorithm(const int socket_fd) -> bool {
        constexpr int one = 1;
        return (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) != -1);
    }

    auto inline set_so_timestamp(const int socket_fd) -> bool {
        constexpr int one = 1;
        return (setsockopt(socket_fd, SOL_SOCKET, SO_TIMESTAMP, &one, sizeof(one)) != -1);
    }

    auto inline join(const int fd, const std::string &ip) -> bool {
        const ip_mreq mreq{{inet_addr(ip.c_str())}, {htonl(INADDR_ANY)}};
        return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1);
    }

    [[nodiscard]] auto inline create_udp_socket(const SocketConfig &config) -> int {
        int sockfd = -1;
        const auto ip = get_interface_ip(config.interface);
        std::cout << "INFO::create_udp_socket Creating UDP socket at " << ip << ":" << config.port << " with config: " << config.to_string() << "\n";

        constexpr addrinfo hints{
            .ai_flags = AI_PASSIVE,
            .ai_family = AF_INET,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP
        };
        addrinfo *res = nullptr;
        if (getaddrinfo(ip.c_str(), std::to_string(config.port).c_str(), &hints, &res) != 0) {
            std::cerr << "ERROR::create_udp_socket getaddrinfo failed for " << ip << ":" << config.port << "\n";
            return -1;
        }

        for (const addrinfo *p = res; p != nullptr; p = p->ai_next) {
            const auto fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd == -1) {
                continue;
            }

            if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &MulticastBufferSize, sizeof(MulticastBufferSize)) == -1) {
                std::cerr << "WARN::create_udp_socket setsockopt(SO_RCVBUF) failed: " << std::strerror(errno) << "\n";
            }

            if (config.is_listening) {
                if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    continue;
                }
            }

            if (config.need_so_timestamp) {
                if (!set_so_timestamp(sockfd)) {
                    std::cerr << "ERROR::create_udp_socket Failed to set SO_TIMESTAMP on socket\n";
                    close(sockfd);
                    continue;
                }
            }
            sockfd = fd;
            break;
        }
        freeaddrinfo(res);
        if (sockfd == -1) {
            std::cerr << "ERROR::create_udp_socket Failed to create UDP socket for " << ip << ":" << config.port << "\n";
            return -1;
        }
        return sockfd;
    }

    [[nodiscard]] auto inline configure_multicast_lo_sender(const int socket_fd) -> bool {
        // 1. set TTL
        // 2. set multicast interface - explicitly set loopback interface
        constexpr int ttl = 1;
        if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
            std::cerr << "ERROR::configure_multicast_lo_sender setsockopt(IP_MULTICAST_TTL) failed: " << std::strerror(errno) << "\n";
            return false;
        }

        in_addr loopback_interface{};
        loopback_interface.s_addr = inet_addr(MulticastLocalHostIP);
        if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_IF,
                       (const char*)&loopback_interface, sizeof(loopback_interface)) < 0) {
            std::cerr << "ERROR::configure_multicast_loopback_sender setsockopt(IP_MULTICAST_IF) failed: " << std::strerror(errno) << "\n";
            return false;
        }
        constexpr char loop = 1;
        if (setsockopt(socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
            std::cerr << "WARN::configure_multicast_loopback_sender setsockopt(IP_MULTICAST_LOOP) failed: " << std::strerror(errno) << "\n";
            // Not critical, continue
        }
        std::cerr << "INFO::configure_multicast_loopback_sender Multicast sender successfully configured for 127.0.0.1\n";
        return true;
    }

    [[nodiscard]] auto inline create_tcp_socket(const SocketConfig &config) -> int {
        std::cerr << "ERROR::create_tcp_socket not implemented yet." << std::endl;
        return -1;
    }

    [[nodiscard]] auto inline create_unix_socket(const SocketConfig &config) -> int {
        std::cerr << "ERROR::create_unix_socket not implemented yet." << std::endl;
        return -1;
    }

    [[nodiscard]] auto inline create_socket(const SocketConfig &config) -> int {
        switch (config.type) {
            case SocketType::TCP:
                return create_tcp_socket(config);
            case SocketType::UDP: {
                const int fd = create_udp_socket(config);
                if (fd == -1) {
                    return fd;
                }
                if (const bool res = configure_multicast_lo_sender(fd); !res) {
                    close(fd);
                    return -1;
                }
                return fd;
            }
            case SocketType::UNIX:
                return create_unix_socket(config);
        }
        return -1;
    }
}