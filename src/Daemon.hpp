#pragma once
#include "Http.hpp"
#include "UnixSocket.hpp"
#include "main.hpp"

int runDaemon(const Args &args);

class Daemon {
  public:
    Daemon();
    ~Daemon();

    void listen();

    int run(bool &);

  private:
    int runCommand(const std::string &command);

    void discoverIGD();
    void getIGDServiceDescription();

    UnixSocket m_socket;

    std::optional<HTTP::URL> m_IGDLocation;
};
