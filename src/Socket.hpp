#pragma once

#include <string>
#include <sys/socket.h>

#define DEFAULT_BACKLOG 5

class Socket {
  public:
    virtual ~Socket();

    virtual void connect(const std::string &, int = 0);
    virtual void bind(const std::string &, int = 0);
    virtual void listen(int = DEFAULT_BACKLOG);
    virtual Socket *accept();
    virtual void close();

    virtual ssize_t send(const uint8_t *, size_t, int = 0);
    virtual ssize_t receive(uint8_t *, size_t, int = 0);

    void setFlags(int);
    void clearFlags(int);

    bool hasFileDescriptor();

  protected:
    Socket(int, int, int = 0, int = 0);
    Socket(int);

    virtual Socket *_accept_fd(int) = 0;
    virtual void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) = 0;

    void _connect(struct sockaddr *, socklen_t);
    void _bind(struct sockaddr *, socklen_t);
    int _accept(struct sockaddr *, socklen_t *);

    int m_sockfd = -1;
};
