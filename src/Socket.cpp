#include "Socket.hpp"
//#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

#include <unordered_map>

Socket::Socket(int domain, int type, int protocol /*, int flags*/) : m_isNonBlocking(false), m_isAddressReuse(false) {
    int sockfd = socket(domain, type, protocol);
    // if (sockfd == -1) {
    if (sockfd < 0) {
        throw std::system_error(errno, std::generic_category());
        // std::cerr << "Failed to create socket." << std::endl;
        // exit(1);
    }
    m_sockfd = sockfd;
    // if (flags != 0)
    //     setFlags(flags);
}

Socket::Socket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : m_sockfd(client_fd), m_isNonBlocking(false), m_isAddressReuse(false) {
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
int Socket::getFileDescriptor() {
    return m_sockfd;
}

void Socket::setAddressReuse(bool enabled) {
    int reuse = (int)enabled;
    setSockOpt(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    m_isAddressReuse = enabled;
}

void Socket::setNonBlocking(bool enabled) {
    if (enabled) {
        setFlags(O_NONBLOCK);
    } else {
        clearFlags(O_NONBLOCK);
    }
    m_isNonBlocking = enabled;
}

bool Socket::isNonBlocking() {
    return m_isNonBlocking;
}

std::optional<std::tuple<std::string, int>> Socket::getAddress() {
    if (!m_hasRemoteAddr)
        return std::nullopt;
    return _unbind_sockaddr(reinterpret_cast<struct sockaddr_storage *>(&m_addr), m_addrlen);
}

/*
static function handling many sockets:
std::pair<Socket::EventType, Socket *> Socket::select(std::initializer_list<Socket *> sockets, long timeout) {
    fd_set readSet;
    fd_set writeSet;
    int maxfd = -1;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    std::unordered_map<int, Socket *> socketsMap;

    for (const auto &i : sockets) {
        if (i == nullptr)
            continue;
        int fd = i->m_sockfd;
        if (fd >= 0 && i->m_isNonBlocking) {
            FD_SET(fd, &readSet);
            FD_SET(fd, &writeSet);

            if (maxfd < fd) {
                maxfd = fd;
            }
            socketsMap[fd] = i;
        }
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int ready = ::select(maxfd + 1, &readSet, &writeSet, nullptr, &tv);

    if (ready < 0) {
        // Error occurred during select()
        throw std::system_error(errno, std::generic_category());
        // return nullptr;
    } else if (ready == 0) {
        // Timeout occurred
        return std::pair<Socket::EventType, Socket*>(EventType::TIMEOUT, nullptr);
    } else {
        // Events are ready, handle them accordingly
        EventType t = EventType::NONE;

        for (int i = 0; i < numFds; ++i) {
            if (FD_ISSET(fds[i], &readSet)) {
                // Read event occurred on fds[i]
                // Handle read event
            }

            if (FD_ISSET(fds[i], &writeSet)) {
                // Write event occurred on fds[i]
                // Handle write event
            }
        }

        return ready;
    }
}*/

Socket::EventType Socket::select(long timeout, bool write, bool read) {
    if (m_sockfd < 0)
        throw std::runtime_error("'select()' called on socket with invalid file descriptor");
    if (!m_isNonBlocking)
        throw std::runtime_error("'select()' called on a blocking socket!");

    fd_set readSet;
    fd_set writeSet;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    if (read)
        FD_SET(m_sockfd, &readSet);
    if (write)
        FD_SET(m_sockfd, &writeSet);

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int ready = ::select(m_sockfd + 1, &readSet, &writeSet, nullptr, &tv);

    if (ready < 0) {
        // Error occurred during select()
        throw std::system_error(errno, std::generic_category());
    } else if (ready == 0) {
        // Timeout occurred
        return EventType::TIMEOUT;
    } else {
        bool isRead = read && FD_ISSET(m_sockfd, &readSet);
        bool isWrite = write && FD_ISSET(m_sockfd, &writeSet);

        if (isRead && isWrite) {
            return EventType::BOTH;
        } else if (isRead) {
            return EventType::READ;
        } else if (isWrite) {
            return EventType::WRITE;
        } else {
            return EventType::NONE;
        }
    }
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
