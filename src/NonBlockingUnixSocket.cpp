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

NonBlockingUnixSocket::NonBlockingUnixSocket() : NonBlockingStreamSocket(AF_UNIX, 0) {
}

NonBlockingUnixSocket::NonBlockingUnixSocket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : NonBlockingStreamSocket(client_fd, addr, addrlen) {
}

StreamSocket *NonBlockingUnixSocket::_accept_fd(int client_fd, struct sockaddr *addr, socklen_t *addrlen) {
    return new UnixSocket(client_fd, addr, addrlen); // the socket returned from a non-blocking socket accept() is a blocking socket
}

void NonBlockingUnixSocket::_bind_sockaddr(const std::string &address, int port, struct sockaddr_storage *addr, socklen_t *addrlen) {
    UNUSED(port);
    sockaddr_un *local = reinterpret_cast<sockaddr_un *>(addr);
    std::memset(local, 0, sizeof(sockaddr_un));

    local->sun_family = AF_UNIX;
    std::strncpy(local->sun_path, address.c_str(), sizeof(local->sun_path) - 1);

    *addrlen = sizeof(sockaddr_un);
}

std::tuple<std::string, int> NonBlockingUnixSocket::_unbind_sockaddr(struct sockaddr_storage *, socklen_t) {
    throw std::runtime_error("Not Implemented");
}
