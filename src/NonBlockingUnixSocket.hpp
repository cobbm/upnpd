#pragma once

#include "NonBlockingSocket.hpp"
#include "UnixSocket.hpp"

class NonBlockingUnixSocket : public NonBlockingSocket {
  public:
    NonBlockingUnixSocket();

  protected:
    // int _accept(struct sockaddr *, socklen_t *) override;

    NonBlockingUnixSocket(int);

    Socket *_accept_fd(int) override;
    void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) override;
};
