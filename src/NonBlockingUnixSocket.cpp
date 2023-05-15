#include "NonBlockingUnixSocket.hpp"
#include <cstring>
#include <string>
#include <sys/un.h>
/*
#include <fcntl.h>
NonBlockingUnixSocket::NonBlockingUnixSocket() : UnixSocket() {
    setFlags(O_NONBLOCK);
}

*/

NonBlockingUnixSocket::NonBlockingUnixSocket() : NonBlockingSocket(AF_UNIX, SOCK_STREAM, 0) {
}

NonBlockingUnixSocket::NonBlockingUnixSocket(int client_fd) : NonBlockingSocket(client_fd) {
}

Socket *NonBlockingUnixSocket::_accept_fd(int client_fd) {
    return new UnixSocket(client_fd); // the socket returned from a non-blocking socket accept() is a blocking socket
}

void NonBlockingUnixSocket::_bind_sockaddr(const std::string &address, int port, struct sockaddr_storage *addr, socklen_t *addrlen) {
    UNUSED(port);
    sockaddr_un *local = reinterpret_cast<sockaddr_un *>(addr);
    std::memset(local, 0, sizeof(sockaddr_un));

    local->sun_family = AF_UNIX;
    std::strncpy(local->sun_path, address.c_str(), sizeof(local->sun_path) - 1);

    *addrlen = sizeof(sockaddr_un);
}
