#include "main.hpp"
#include "Client.hpp"
#include "Daemon.hpp"
#include "UnixSocket.hpp"
#include <iostream>
#include <unistd.h>

int parseArgs(Args &args, int argc, char *argv[]) {
    int i = 1;

    while (i < argc) {
        if (std::strcmp(argv[i], "-d") == 0) {
            args.asDaemon = true;
        } else if (std::strcmp(argv[i], "-c") == 0) {
            args.asClient = true;
            if (i + 1 < argc) {
                i++;
                args.clientData = std::string(argv[i]);
            }
        } else {
            std::cerr << "Invalid argument: " << argv[i] << std::endl;
            return 1;
        }
        i++;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    Args args;
    if (parseArgs(args, argc, argv))
        return 1;

    if (args.asDaemon) {
        std::cout << "Running as daemon..." << std::endl;
        return runDaemon(args);
    } else if (args.asClient) {
        std::cout << "Running as client..." << std::endl;
        return runClient(args);
    } else {
        std::cerr << "Invalid arguments" << std::endl;
        return 1;
    }
}
