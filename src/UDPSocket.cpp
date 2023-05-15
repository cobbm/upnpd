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
