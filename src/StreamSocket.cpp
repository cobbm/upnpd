#include "StreamSocket.hpp"

#include <system_error>

StreamSocket::StreamSocket(int domain, int protocol, int flags) : Socket(domain, SOCK_STREAM, protocol, flags) {
}

StreamSocket::StreamSocket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : Socket(client_fd, addr, addrlen) {
}

void StreamSocket::connect(const std::string &address, int port) {
    struct sockaddr_storage addr;
    socklen_t addrlen;

    _bind_sockaddr(address, port, &addr, &addrlen);
    _connect(reinterpret_cast<sockaddr *>(&addr), addrlen);
}

void StreamSocket::bind(const std::string &address, int port) {
    struct sockaddr_storage addr;
    socklen_t addrlen;

    _bind_sockaddr(address, port, &addr, &addrlen);
    _bind(reinterpret_cast<sockaddr *>(&addr), addrlen);
}

void StreamSocket::listen(int backlog) {
    if (m_sockfd < 0)
        throw std::runtime_error("'listen()' called on closed socket");

    if (::listen(m_sockfd, backlog) < 0)
        throw std::system_error(errno, std::generic_category());
}

StreamSocket *StreamSocket::accept() {
    struct sockaddr remote_addr;
    socklen_t remote_addrlen;

    int remote_fd = _accept(&remote_addr, &remote_addrlen);
    if (remote_fd < 0)
        return nullptr;

    return _accept_fd(remote_fd, &remote_addr, &remote_addrlen);
}

ssize_t StreamSocket::send(const uint8_t *buff, size_t len, int flags) {
    int rLen = ::send(m_sockfd, buff, len, flags);
    if (rLen < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t StreamSocket::receive(uint8_t *buff, size_t len, int flags) {
    int rLen = ::recv(m_sockfd, buff, len, flags);
    if (rLen < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t StreamSocket::send(const std::vector<uint8_t> &data) {
    return (size_t)send(data.data(), data.size());
}

ssize_t StreamSocket::receive(std::vector<uint8_t> &data) {
    ssize_t l = receive(&data[0], data.capacity());
    ssize_t newLength = l > 0 ? l : 0;
    data.resize(newLength);

    return l;
}

void StreamSocket::_connect(struct sockaddr *addr, socklen_t addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'connect()' called on closed socket");

    if (::connect(m_sockfd, addr, addrlen) < 0)
        throw std::system_error(errno, std::generic_category());
}

void StreamSocket::_bind(struct sockaddr *addr, socklen_t addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'bind()' called on closed socket");

    if (::bind(m_sockfd, addr, addrlen) < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
}

int StreamSocket::_accept(struct sockaddr *addr, socklen_t *addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'accept()' called on closed socket");

    int client_sockfd = ::accept(m_sockfd, addr, addrlen);

    if (client_sockfd < 0)
        throw std::system_error(errno, std::generic_category());

    return client_sockfd;
}
