#pragma once

#include <optional>
#include <string>
#include <sys/socket.h>
#include <tuple>
#include <stdexcept>

//#ifdef __linux__
//#include <sys/epoll.h>
//#endif

#define UNUSED(__x__) (void)__x__

class Socket {
  public:
    virtual ~Socket();

    enum class EventType { NONE, READ, WRITE, BOTH, TIMEOUT };

    virtual void close();

    int getFileDescriptor();

    void setAddressReuse(bool);
    void setNonBlocking(bool);

    bool isNonBlocking();

    std::optional<std::tuple<std::string, int>> getAddress();

    // static std::pair<Socket::EventType, Socket *> select(std::initializer_list<Socket *>, long);
    Socket::EventType select(long, bool = true, bool = true);

  protected:
    Socket(int, int, int = 0 /*, int = 0*/);
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

    bool m_isNonBlocking;
    bool m_isAddressReuse;
};

/*
#ifdef __linux__
class SocketPoller {
    SocketPoller(std::initializer_list<Socket &> sockets) {
        int epfd = epoll_create(1);
        m_event.events = EPOLLIN; // Can append "|EPOLLOUT" for write events as well
        m_event.data.fd = sockfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
    }

    ~SocketPoller();

  private:
    int m_epollfd;
    struct epoll_event m_event;
};
#endif
*/
