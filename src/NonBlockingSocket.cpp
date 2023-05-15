#include "NonBlockingSocket.hpp"
//#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

NonBlockingSocket::NonBlockingSocket(int domain, int type, int protocol, int flags) : Socket(domain, type, protocol, flags | O_NONBLOCK) {
}

NonBlockingSocket::NonBlockingSocket(int client_fd) : Socket(client_fd) {
    setFlags(O_NONBLOCK);
}
int NonBlockingSocket::_accept(struct sockaddr *addr, socklen_t *addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'accept()' called on closed socket");

    int client_sockfd = ::accept(m_sockfd, addr, addrlen);

    if (client_sockfd < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        throw std::system_error(errno, std::generic_category());
    }

    return client_sockfd;
}

ssize_t NonBlockingSocket::send(const uint8_t *buff, size_t len, int flags) {
    int rLen = ::send(m_sockfd, buff, len, flags);
    if (rLen < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t NonBlockingSocket::receive(uint8_t *buff, size_t len, int flags) {
    int rLen = ::recv(m_sockfd, buff, len, flags);
    if (rLen < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}
