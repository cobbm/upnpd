#pragma once

#include "Socket.hpp"

class UnixSocket : public Socket {
  public:
    UnixSocket();

  protected:
    UnixSocket(int);

    Socket *_accept_fd(int) override;
    void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) override;

  private:
};
