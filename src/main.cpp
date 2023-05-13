#include "UnixSocket.hpp"
#include <iostream>

#define SOCK_PATH "/home/pi/projects/upnpd/my_echo_socket"

#define UNUSED(__x__) (void)__x__

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    UnixSocket s;
    // s.bind(SOCK_PATH);
    s.connect(SOCK_PATH);

    std::cout << "Connected!" << std::endl;

    while (true) {
    }
}
