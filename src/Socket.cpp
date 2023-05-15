#include "Socket.hpp"
//#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

Socket::Socket(int domain, int type, int protocol, int flags) {
    int sockfd = socket(domain, type, protocol);
    // if (sockfd == -1) {
    if (sockfd < 0) {
        throw std::system_error(errno, std::generic_category());
        // std::cerr << "Failed to create socket." << std::endl;
        // exit(1);
    }
    m_sockfd = sockfd;
    if (flags != 0)
        setFlags(flags);
}

Socket::Socket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : m_sockfd(client_fd) {
    if (m_sockfd < 0)
        throw std::runtime_error("Invalid file descriptor");

    if (addr != nullptr && addrlen != nullptr) {
        m_addr = *addr;
        m_addrlen = *addrlen;
        m_hasRemoteAddr = true;
    } else if (addr != nullptr || addrlen != nullptr) {
        throw std::runtime_error("Invalid remote address for socket");
    }
}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (m_sockfd >= 0) {
        int closeResult = ::close(m_sockfd);
        UNUSED(closeResult);
        m_sockfd = -1;
    }
}

/*
ssize_t Socket::send(const uint8_t *buff, size_t len, int flags) {
    int rLen = ::send(m_sockfd, buff, len, flags);
    if (rLen < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t Socket::receive(uint8_t *buff, size_t len, int flags) {
    int rLen = ::recv(m_sockfd, buff, len, flags);
    if (rLen < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
}

ssize_t Socket::send(const std::vector<uint8_t> &data) {
    return (size_t)send(data.data(), data.size());
}

ssize_t Socket::receive(std::vector<uint8_t> &data) {
    ssize_t l = receive(&data[0], data.capacity());
    ssize_t newLength = l > 0 ? l : 0;
    data.resize(newLength);

    return l;
}
*/
bool Socket::hasFileDescriptor() {
    return m_sockfd >= 0;
}

void Socket::setAddressReuse(bool enabled) {
    int reuse = (int)enabled;
    setSockOpt(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

void Socket::setFlags(int flags) {
    int fd_flags = fcntl(m_sockfd, F_GETFL, 0);
    if (fd_flags < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }

    fd_flags |= flags;

    int setResult = fcntl(m_sockfd, F_SETFL, fd_flags);
    if (setResult < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
}

void Socket::clearFlags(int flags) {
    int fd_flags = fcntl(m_sockfd, F_GETFL, 0);
    if (fd_flags < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }

    fd_flags &= ~flags;

    int setResult = fcntl(m_sockfd, F_SETFL, fd_flags);
    if (setResult < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
}

void Socket::setSockOpt(int level, int option_name, const void *option_value, socklen_t option_len) {
    int setResult = setsockopt(m_sockfd, level, option_name, option_value, option_len);
    if (setResult < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
}

/*
void Socket::_connect(struct sockaddr *addr, socklen_t addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'connect()' called on closed socket");

    if (::connect(m_sockfd, addr, addrlen) < 0)
        throw std::system_error(errno, std::generic_category());
}

void Socket::_bind(struct sockaddr *addr, socklen_t addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'bind()' called on closed socket");

    if (::bind(m_sockfd, addr, addrlen) < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
}

int Socket::_accept(struct sockaddr *addr, socklen_t *addrlen) {
    if (m_sockfd < 0)
        throw std::runtime_error("'accept()' called on closed socket");

    int client_sockfd = ::accept(m_sockfd, addr, addrlen);

    if (client_sockfd < 0)
        throw std::system_error(errno, std::generic_category());

    return client_sockfd;
}*/
