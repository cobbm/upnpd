#pragma once

#include "Socket.hpp"
#include <string>
#include <sys/socket.h>
#include <vector>

#define DEFAULT_BACKLOG 5

class StreamSocket : public Socket {
  public:
    // virtual ~Socket();

    virtual void connect(const std::string &, int = 0);
    virtual void bind(const std::string &, int = 0);
    virtual void listen(int = DEFAULT_BACKLOG);
    virtual StreamSocket *accept();

    // virtual void close();

    virtual ssize_t send(const uint8_t *, size_t, int = 0);
    virtual ssize_t receive(uint8_t *, size_t, int = 0);

    ssize_t send(const std::vector<uint8_t> &);
    ssize_t receive(std::vector<uint8_t> &);

  protected:
    StreamSocket(int, int = 0, int = 0);
    StreamSocket(int, struct sockaddr *, socklen_t *);

    virtual StreamSocket *_accept_fd(int, struct sockaddr *, socklen_t *) = 0;

    void _connect(struct sockaddr *, socklen_t);
    void _bind(struct sockaddr *, socklen_t);
    virtual int _accept(struct sockaddr *, socklen_t *);
};
