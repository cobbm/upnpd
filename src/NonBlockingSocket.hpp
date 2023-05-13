#pragma once

#include "Socket.hpp"

class NonBlockingSocket : public Socket {
  protected:
    NonBlockingSocket(int, int, int = 0, int = 0);
    NonBlockingSocket(int);
};
