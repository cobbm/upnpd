#pragma once

#include <optional>
#include <string>
#include <sys/socket.h>
#include <tuple>

#define UNUSED(__x__) (void)__x__

class Socket {
  public:
    virtual ~Socket();

    virtual void close();

    bool hasFileDescriptor();
    void setAddressReuse(bool);

    std::optional<std::tuple<std::string, int>> getAddress();

  protected:
    Socket(int, int, int = 0, int = 0);
    Socket(int, struct sockaddr *, socklen_t *);

    void setFlags(int);
    void clearFlags(int);
    void setSockOpt(int, int, const void *, socklen_t);

    // virtual Socket *_accept_fd(int, struct sockaddr *, socklen_t *) = 0;
    virtual void _bind_sockaddr(const std::string &, int, struct sockaddr_storage *, socklen_t *) = 0;
    virtual std::tuple<std::string, int> _unbind_sockaddr(struct sockaddr_storage *, socklen_t) = 0;

    int m_sockfd = -1;

    bool m_hasRemoteAddr = false;
    struct sockaddr m_addr;
    socklen_t m_addrlen;
};
