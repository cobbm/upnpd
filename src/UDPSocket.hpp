#pragma once

#include "DgramSocket.hpp"

class UDPSocket : public DgramSocket {
  public:
    UDPSocket();

    ssize_t sendTo(const std::string &, int, const uint8_t *, size_t, int = 0);
    ssize_t receiveFrom(const std::string &, int, uint8_t *, size_t, int = 0);

    ssize_t sendTo(const std::string &, int, const std::vector<uint8_t> &);
    ssize_t receiveFrom(const std::string &, int, std::vector<uint8_t> &);

  protected:
    UDPSocket(int, struct sockaddr *, socklen_t *);

    // Socket *_accept_fd(int, struct sockaddr *, socklen_t *) override;
    void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) override;

  private:
};
