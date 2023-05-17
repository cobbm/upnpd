#pragma once

#include "Socket.hpp"
#include <string>
#include <sys/socket.h>
#include <tuple>
#include <vector>

class DgramSocket : public Socket {
  public:
    // DGramSocket();

    ssize_t sendTo(const std::string &, int, const uint8_t *, size_t, int = 0);
    std::tuple<ssize_t, std::string, int> receiveFrom(uint8_t *, size_t, int = 0);

    ssize_t sendTo(const std::string &, int, const std::vector<uint8_t> &);
    std::tuple<ssize_t, std::string, int> receiveFrom(std::vector<uint8_t> &);

  protected:
    DgramSocket(int, int = 0 /*, int = 0*/);
    DgramSocket(int, struct sockaddr *, socklen_t *);

    // Socket *_accept_fd(int, struct sockaddr *, socklen_t *) override;
    // virtual void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) = 0;
};
