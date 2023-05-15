#pragma once

#include "NonBlockingStreamSocket.hpp"
#include "UnixSocket.hpp"

class NonBlockingUnixSocket : public NonBlockingStreamSocket {
  public:
    NonBlockingUnixSocket();

  protected:
    // int _accept(struct sockaddr *, socklen_t *) override;

    NonBlockingUnixSocket(int, struct sockaddr *, socklen_t *);

    StreamSocket *_accept_fd(int, struct sockaddr *, socklen_t *) override;
    void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) override;

    std::tuple<std::string, int> _unbind_sockaddr(struct sockaddr_storage *, socklen_t)
    override;
};
