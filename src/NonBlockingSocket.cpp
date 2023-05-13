#include "NonBlockingSocket.hpp"
#include <fcntl.h>
NonBlockingSocket::NonBlockingSocket(int domain, int type, int protocol, int flags) : Socket(domain, type, protocol, flags | O_NONBLOCK) {
}

NonBlockingSocket::NonBlockingSocket(int client_fd) : Socket(client_fd) {
    setFlags(O_NONBLOCK);
}
