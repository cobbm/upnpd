#include "Daemon.hpp"

#include "NonBlockingUnixSocket.hpp"
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

        bool hasCommand = true;
        CommandType c = CommandType::NONE;
        {
            std::unique_lock<std::mutex> lock(listenerMutex);

            // Wait until data is available or timeout occurs
            if (listenerCV.wait_for(lock, std::chrono::seconds(15), [] { return listenerDataAvailable || daemonExit || daemonError; })) {

                if (listenerDataAvailable) {
                    std::cout << "[Worker] Data received from main thread!" << std::endl;
                    c = listenerCommand;
                    listenerDataAvailable = false;
                }
            } else {
                hasCommand = false;
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
