#include "Client.hpp"
#include "UnixSocket.hpp"
#include <iostream>
#include <unistd.h>
#include <vector>

int runClient(const Args &args) {
    UnixSocket s;
    s.connect(SOCK_PATH);

    std::cout << "Connected!" << std::endl;

    std::string dataStr;
    if (args.clientData.size()) {
        dataStr = args.clientData;
    } else {
        dataStr = "Hello World!";
    }
    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());
    s.send(data);

    return 0;
}
