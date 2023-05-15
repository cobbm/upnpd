#pragma once

#include "Socket.hpp"

class NonBlockingSocket : public Socket {
  public:
    // Socket *accept() override;

  protected:
    NonBlockingSocket(int, int, int = 0, int = 0);
    NonBlockingSocket(int);

    int _accept(struct sockaddr *, socklen_t *) override;

    ssize_t send(const uint8_t *, size_t, int) override;
    ssize_t receive(uint8_t *, size_t, int) override;
};
