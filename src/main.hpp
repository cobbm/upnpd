#pragma once
#include <string>

//#define SOCK_PATH "/home/pi/projects/upnpd/my_echo_socket"
#define SOCK_PATH "/tmp/upnpd_socket"

struct Args {
    bool asDaemon = false;
    bool asClient = false;
    std::string clientData;
};
