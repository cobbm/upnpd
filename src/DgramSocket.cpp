#include "DgramSocket.hpp"

#include <arpa/inet.h>
#include <system_error>

DgramSocket::DgramSocket(int domain, int protocol, int flags) : Socket(domain, SOCK_DGRAM, protocol, flags) {
}

DgramSocket::DgramSocket(int client_fd, struct sockaddr *addr, socklen_t *addrlen) : Socket(client_fd, addr, addrlen) {
}

ssize_t DgramSocket::sendTo(const std::string &address, int port, const uint8_t *buff, size_t len, int flags) {
    /*
    int rLen = ::send(m_sockfd, buff, len, flags);
    if (rLen < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return rLen;
    */
    sockaddr_in remoteAddr;
    socklen_t addrlen;

    _bind_sockaddr(address, port, reinterpret_cast<sockaddr_storage *>(&remoteAddr), &addrlen);

    int sendResult = ::sendto(m_sockfd, buff, len, flags, reinterpret_cast<sockaddr *>(&remoteAddr), addrlen);

    if (sendResult < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return sendResult;
}
std::tuple<ssize_t, std::string, int> DgramSocket::receiveFrom(uint8_t *buff, size_t len, int flags) {
    sockaddr_in remoteAddr;
    socklen_t addrlen;

    //_bind_sockaddr(address, port, reinterpret_cast<sockaddr_storage *>(&remoteAddr), &addrlen);

    int receiveResult = ::recvfrom(m_sockfd, buff, len, flags, reinterpret_cast<sockaddr *>(&remoteAddr), &addrlen);

    auto from = _unbind_sockaddr(reinterpret_cast<sockaddr_storage *>(&remoteAddr), addrlen);

    if (receiveResult < 0) {
        // close();
        throw std::system_error(errno, std::generic_category());
    }
    return std::tuple<ssize_t, std::string, int>(receiveResult, std::get<0>(from), std::get<1>(from));
}

ssize_t DgramSocket::sendTo(const std::string &address, int port, const std::vector<uint8_t> &data) {
    return (size_t)sendTo(address, port, data.data(), data.size());
}

std::tuple<ssize_t, std::string, int> DgramSocket::receiveFrom(std::vector<uint8_t> &data) {
    auto l = receiveFrom(&data[0], data.capacity());
    ssize_t newLength = std::get<0>(l) > 0 ? std::get<0>(l) : 0;
    data.resize(newLength);

    return l;
}
