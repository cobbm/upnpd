#pragma once

#include "StreamSocket.hpp"

class NonBlockingStreamSocket : public StreamSocket {
  public:
    // Socket *accept() override;

  protected:
    NonBlockingStreamSocket(int, int = 0, int = 0);
    NonBlockingStreamSocket(int, struct sockaddr *, socklen_t *);

    int _accept(struct sockaddr *, socklen_t *) override;

    ssize_t send(const uint8_t *, size_t, int) override;
    ssize_t receive(uint8_t *, size_t, int) override;
};
