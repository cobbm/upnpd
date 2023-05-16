#include "UDPSocket.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <system_error>

UDPSocket::UDPSocket() : DgramSocket(AF_INET, 0) {
}

UDPSocket::UDPSocket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : DgramSocket(client_fd, addr, addrlen) {
}

/*
Socket *UDPSocket::_accept_fd(int client_fd, struct sockaddr *addr, socklen_t *addrlen) {
    return new UDPSocket(client_fd, addr, addrlen);
}*/

void UDPSocket::_bind_sockaddr(const std::string &address, int port, struct sockaddr_storage *addr, socklen_t *addrlen) {
    sockaddr_in *local = reinterpret_cast<sockaddr_in *>(addr);
    std::memset(local, 0, sizeof(sockaddr_in));

    local->sin_family = AF_INET;
    local->sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &(local->sin_addr));

    *addrlen = sizeof(sockaddr_in);
}

std::tuple<std::string, int> UDPSocket::_unbind_sockaddr(struct sockaddr_storage *addr, socklen_t addrlen) {
    if (addr->ss_family == AF_INET) {
        sockaddr_in *_addr = reinterpret_cast<sockaddr_in *>(addr);

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(_addr->sin_addr), str, INET_ADDRSTRLEN);
        return std::tuple<std::string, int>(std::string(str), ntohs(_addr->sin_port));
    } else {
        sockaddr_in6 *_addr = reinterpret_cast<sockaddr_in6 *>(addr);

        char str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(_addr->sin6_addr), str, INET6_ADDRSTRLEN);
        return std::tuple<std::string, int>(std::string(str), ntohs(_addr->sin6_port));
    }
}
