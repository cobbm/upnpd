#include "UnixSocket.hpp"
#include <cstring>
#include <string>
#include <sys/un.h>

UnixSocket::UnixSocket() : Socket(AF_UNIX, SOCK_STREAM, 0) {
}

UnixSocket::UnixSocket(int client_fd) : Socket(client_fd) {
}

Socket *UnixSocket::_accept_fd(int client_fd) {
    return new UnixSocket(client_fd);
}

// void UnixSocket::_bind_sockaddr(const std::string &address, int port, sockaddr_storage *addr, socklen_t *addrlen) {
void UnixSocket::_bind_sockaddr(const std::string &address, int port, struct sockaddr_storage *addr, socklen_t *addrlen) {
    UNUSED(port);
    sockaddr_un *local = reinterpret_cast<sockaddr_un *>(addr);
    std::memset(local, 0, sizeof(sockaddr_un));

    local->sun_family = AF_UNIX;
    std::strncpy(local->sun_path, address.c_str(), sizeof(local->sun_path) - 1);

    *addrlen = sizeof(sockaddr_un);
}
