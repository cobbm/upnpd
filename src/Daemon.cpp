#include "Daemon.hpp"

#include "NonBlockingUnixSocket.hpp"
#include "UDPSocket.hpp"
#include "UnixSocket.hpp"
#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>

void upnpDiscover() {

    std::cout << "[Worker] Discovering UPnP devices..." << std::endl;

    // send UDP on broadcast 239.255.255.250:1900

    // Create a UDP socket for sending the SSDP message
    UDPSocket s;
    // Enable reuse of the socket address
    s.setAddressReuse(true);
    // Set up the destination address and port for sending
    std::string searchIp = "239.255.255.250";
    int searchPort = 1900;
    // Prepare the SSDP M-SEARCH message
    std::string searchMsg = "M-SEARCH * HTTP/1.1\r\n"
                            "Host: 239.255.255.250:1900\r\n"
                            "ST: ssdp:all\r\n"
                            "Man: \"ssdp:discover\"\r\n"
                            "MX: 3\r\n\r\n";

    std::vector<uint8_t> msgVector(searchMsg.begin(), searchMsg.end());

    std::vector<uint8_t> reply;
    reply.clear();
    reply.resize(2048);

    // Send the SSDP multicast message
    int sentBytes = s.sendTo(searchIp, searchPort, msgVector);
    if (sentBytes <= 0) {
        std::cerr << "Failed to send SSDP message :(" << std::endl;
        return;
    }

    // Wait for the response

    std::tuple<ssize_t, std::string, int> reply_t = s.receiveFrom(reply);
    if (std::get<0>(reply_t) <= 0) {
        std::cerr << "No reply!" << std::endl;
        return;
    }

    std::cout << "Reply from " << std::get<1>(reply_t) << ":" << std::to_string(std::get<2>(reply_t)) << " (" << std::to_string(reply.size()) << " bytes)"
              << std::endl;
    std::string reply_str(reply.begin(), reply.end());
    std::cout << reply_str << std::endl;

    std::cout << "Discover done" << std::endl;
}

//////////////////////////////////////////////////////

enum class CommandType {
    NONE,
    DISCOVER,
};

// daemon globals for interrupts:
static std::atomic<bool> daemonExit(false);
std::atomic<int> daemonError(0);

std::mutex listenerMutex;
std::condition_variable listenerCV;
// TODO: use queue
enum CommandType listenerCommand = CommandType::NONE;
bool listenerDataAvailable = false;

void sigint_handler(int signum) {
    std::cout << "Caught signal '" << signum << "', shutting down..." << std::endl;
    daemonExit.store(true);
}

// listens for data from client over UNIX socket
void listenerThread() {
    // clean up socket
    unlink(SOCK_PATH);

    // set up UNIX socket listener:
    NonBlockingUnixSocket s;

    std::cout << "[Listener] Binding to socket '" << SOCK_PATH << "'..." << std::endl;
    s.bind(SOCK_PATH);

    std::cout << "[Listener] Listening..." << std::endl;
    s.listen();

    std::vector<uint8_t> buff;

    while (!daemonExit && !daemonError) {
        // TODO: use select() instead
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::unique_ptr<StreamSocket> client(s.accept());
        if (!client) {
            continue;
        }

        // clear buffer
        buff.resize(1024);

        int receivedLength = client->receive(buff);
        if (receivedLength < 0) {
            std::cerr << "[Listener] Client error" << std::endl;
            client->close();
            continue;
        } else if (receivedLength < 1) {
            std::cerr << "[Listener] Client disconnected" << std::endl;
            client->close();
            continue;
        }

        client->close();

        std::string data(buff.begin(), buff.end());

        std::cout << "[Listener] Received (" << data.size() << "): " << data << std::endl;

        // notify the worker thread
        {
            std::lock_guard<std::mutex> lock(listenerMutex);
            if (data == "discover") {
                listenerCommand = CommandType::DISCOVER;
            } else {
                listenerCommand = CommandType::NONE;
            }
            listenerDataAvailable = true;
        }
        listenerCV.notify_one();
    }

    listenerCV.notify_all();

    s.close();

    // clean up socket
    unlink(SOCK_PATH);

    std::cout << "[Listener] Done" << std::endl;
}

// does UPnP stuff
void workerThread() {
    while (!daemonExit && !daemonError) {

        std::cout << "[Worker] Waiting..." << std::endl;

        bool hasCommand = false;
        CommandType c = CommandType::NONE;
        {
            std::unique_lock<std::mutex> lock(listenerMutex);

            // Wait until data is available or timeout occurs
            if (listenerCV.wait_for(lock, std::chrono::seconds(15), [] { return listenerDataAvailable || daemonExit || daemonError; })) {

                if (listenerDataAvailable) {
                    std::cout << "[Worker] Data received from main thread!" << std::endl;
                    c = listenerCommand;
                    hasCommand = true;

                    listenerDataAvailable = false;
                }
            }
        }

        if (hasCommand) {
            if (listenerCommand == CommandType::DISCOVER) {
                upnpDiscover();
            }
        } else {
            std::cout << "[Worker] Timeout" << std::endl;
        }
    }

    std::cout << "[Worker] Done" << std::endl;
}

int runDaemon(const Args &args) {
    // catch SIGINT
    std::signal(SIGINT, sigint_handler);

    std::thread upnpWorkerThread(workerThread);

    listenerThread();

    upnpWorkerThread.join();

    std::cout << "Daemon exiting with code '" << daemonError << "'" << std::endl;
    return daemonError;
}

/*
int runDaemonBlocking(const Args &args) {
    // catch SIGINT
    signal(SIGINT, sigint_handler);

    // clean up socket
    unlink(SOCK_PATH);

    // set up UNIX socket listener:
    UnixSocket s;
    std::cout << "Binding to socket '" << SOCK_PATH << "'..." << std::endl;
    s.bind(SOCK_PATH);
    std::cout << "Listening..." << std::endl;
    s.listen();

    std::vector<uint8_t> buff;

    while (!daemonExit && !daemonError) {
        // sleep(1000);
        std::cout << "Waiting for connection..." << std::endl;
        Socket *client = s.accept();
        if (!client) {
            std::cerr << "Client connection was null" << std::endl;
            continue;
        }

        // clear buffer
        // buff.clear();
        buff.resize(1024);

        if (client->receive(buff) <= 0) {
            std::cerr << "Client disconnected" << std::endl;
            client->close();
            continue;
        }

        std::string data(buff.begin(), buff.end());

        std::cout << "Received (" << data.size() << "): " << data << std::endl;
        // ...
        client->close();
    }

    std::cout << "Daemon exiting with code '" << daemonError << "'" << std::endl;
    s.close();
    // clean up socket
    unlink(SOCK_PATH);

    return daemonError;
}
*/
