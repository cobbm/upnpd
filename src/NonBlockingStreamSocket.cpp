#include "NonBlockingStreamSocket.hpp"
//#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

NonBlockingStreamSocket::NonBlockingStreamSocket(int domain, int protocol, int flags) : StreamSocket(domain, protocol, flags | O_NONBLOCK) {
}

NonBlockingStreamSocket::NonBlockingStreamSocket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : StreamSocket(client_fd, addr, addrlen) {
    setFlags(O_NONBLOCK);
}

int NonBlockingStreamSocket::_accept(struct sockaddr *addr, socklen_t *addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'accept()' called on closed socket");

    int client_sockfd = ::accept(m_sockfd, addr, addrlen);

    if (client_sockfd < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        throw std::system_error(errno, std::generic_category());
    }

    return client_sockfd;
}

ssize_t NonBlockingStreamSocket::send(const uint8_t *buff, size_t len, int flags) {
    int rLen = ::send(m_sockfd, buff, len, flags);
    if (rLen < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t NonBlockingStreamSocket::receive(uint8_t *buff, size_t len, int flags) {
    int rLen = ::recv(m_sockfd, buff, len, flags);
    if (rLen < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}
