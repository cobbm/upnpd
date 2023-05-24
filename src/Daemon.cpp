#include "Daemon.hpp"

#include "UDPSocket.hpp"
#include "UnixSocket.hpp"
// #include <atomic>
#include <csignal>
#include <iostream>
// #include <thread>
#include <algorithm>
#include <memory>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "Http.hpp"

#define LISTENER_SELECT_TIMEOUT 15
#define WORKER_CV_TIMEOUT 15

#define BUFFER_SIZE 2048

static bool daemonExit(false);
int daemonError(0);

void sigint_handler(int signum) {
    std::cout << "Caught signal '" << signum << "', interrupted..." << std::endl;
    daemonExit = true;
}

HTTP::Response makeSSDPRequest(const std::string &ip, int port, const std::string &method, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, long timeout = 10) {

    std::string host = ip + ":" + std::to_string(port);
    std::cout << "[Listener] Making SSDP request to " << host << std::endl;

    std::vector<uint8_t> replyBuff;
    replyBuff.clear();
    replyBuff.resize(BUFFER_SIZE);

    // Create a UDP socket for sending the SSDP message
    UDPSocket s;

    // Enable reuse of the socket address
    s.setAddressReuse(true);
    s.setNonBlocking(true);

    // construct the message:
    std::vector<uint8_t> sendBuff;
    // serialiseSSDPRequest(sendBuff, ip, port, method, path, headers);
    HTTP::Request req(method, path, host);
    for (const auto &i : headers) {
        req.addHeader(i.first, i.second);
    }
    req.build(sendBuff);

    // Send the SSDP multicast message
    int sentBytes = s.sendTo(ip, port, sendBuff);
    if (sentBytes <= 0) {
        throw std::runtime_error("Failed to send UDP message to " + host);
    }

    // Wait for the response
    Socket::EventType reply = s.select(timeout, false);

    if (reply != Socket::EventType::READ) {
        throw std::runtime_error("'select()' call timeout!");
    }

    std::tuple<ssize_t, std::string, int> reply_t = s.receiveFrom(replyBuff);
    if (std::get<0>(reply_t) <= 0) {
        throw std::runtime_error("No reply from " + host);
    }

    std::cout << "[Listener] SSDP Reply from " << std::get<1>(reply_t) << ":" << std::to_string(std::get<2>(reply_t)) << " ("
              << std::to_string(replyBuff.size()) << " bytes)" << std::endl;

    // parse & return the response
    return HTTP::Response(replyBuff);
}

void upnpDiscover() {

    std::cout << "[Listener] Discovering UPnP devices..." << std::endl;

    std::unordered_map<std::string, std::string> headers({ { "ST", "ssdp:all" }, { "Man", "\"ssdp:discover\"" }, { "MX", "3" } });

    auto response = makeSSDPRequest("239.255.255.250", 1900, "M-SEARCH", "*", headers);

    std::cout << "[Listener] SSDP Response status code: " << response.getStatusCode() << std::endl;

    if (!response.ok())
        throw std::runtime_error("Received non-ok response code " + std::to_string(response.getStatusCode()));

    auto loc = response.getHeader("location");
    if (!loc.has_value())
        throw std::runtime_error("Missing 'location' header in response");

    std::cout << "[Listener] UPnP device location: '" << loc.value() << "'" << std::endl;

    std::cout << "[Listener] Discover done" << std::endl;
}

bool waitForData(StreamSocket &s, std::vector<uint8_t> &buff, long timeout) {
    Socket::EventType ready = Socket::EventType::NONE;
    try {
        ready = s.select(timeout);
    } catch (std::system_error &ex) {
        if (ex.code().value() != 4 /* Interrupted System Call */) {
            std::cerr << "[Listener] select() error: " << ex.code().value() << std::endl;
            throw ex;
        }
    }

    if (ready != Socket::EventType::READ)
        return false;

    std::unique_ptr<StreamSocket> client(s.accept());

    if (!client)
        return false;

    // clear buffer
    buff.resize(BUFFER_SIZE);

    int receivedLength = client->receive(buff);
    if (receivedLength < 0) {
        std::cerr << "[Listener] Client error" << std::endl;
        client->close();
        return false;
    } else if (receivedLength < 1) {
        std::cerr << "[Listener] Client disconnected" << std::endl;
        client->close();
        return false;
    }

    client->close();
    return true;
}

void runCommand(const std::string &data) {
    if (data == "discover") {
        upnpDiscover();
    } else {
        std::cerr << "[Listener] Invalid command: '" << data << "'" << std::endl;
    }
}

int runDaemon(const Args &args) {
    // catch SIGINT
    signal(SIGINT, sigint_handler);

    // clean up socket
    unlink(SOCK_PATH);

    // set up UNIX socket listener:
    UnixSocket s;
    s.setNonBlocking(true);

    std::cout << "[Listener] Binding to socket '" << SOCK_PATH << "'..." << std::endl;
    s.bind(SOCK_PATH);

    std::cout << "[Listener] Listening..." << std::endl;
    s.listen();

    std::vector<uint8_t> buff;

    // main loop
    while (!daemonExit && !daemonError) {

        try {
            std::cout << "[Listener] Waiting for data..." << std::endl;

            if (waitForData(s, buff, LISTENER_SELECT_TIMEOUT)) {

                std::string data(buff.begin(), buff.end());

                std::cout << "[Listener] Received (" << data.size() << "): " << data << std::endl;

                runCommand(data);
            }
        } catch (const std::exception &ex) {
            std::cerr << "[Listener] Exception: " << ex.what() << std::endl;
            // TODO: handle excptions??
            throw;
        }
    }

    s.close();

    // clean up socket
    unlink(SOCK_PATH);

    std::cout << "[Listener] Done, daemon exiting with code '" << daemonError << "'" << std::endl;

    return daemonError;
}

/*
enum class CommandType {
    NONE,
    WAKEUP,
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
    std::cout << "Caught signal '" << signum << "', interrupted..." << std::endl;
    daemonExit.store(true);
}

void listenerSendToWorker(CommandType c) {
    // notify the worker thread
    {
        std::lock_guard<std::mutex> lock(listenerMutex);

        listenerCommand = c;

        listenerDataAvailable = true;
    }
    listenerCV.notify_all();
}

bool listenerParseCommand(const std::string &command, CommandType &outCommand) {
    if (command == "discover") {
        outCommand = CommandType::DISCOVER;
    } else if (command == "wakeup") {
        outCommand = CommandType::WAKEUP;
    } else {
        outCommand = CommandType::NONE;
        return false;
    }
    return true;
}

bool listenerWaitForData(StreamSocket &s, std::vector<uint8_t> &buff) {
    Socket::EventType ready = Socket::EventType::NONE;
    try {
        ready = s.select(LISTENER_SELECT_TIMEOUT);
    } catch (std::system_error &ex) {
        if (ex.code().value() != 4 / * Interrupted System Call * /) {
            std::cerr << "[Listener] select() error: " << ex.code().value() << std::endl;
            throw ex;
        }
    }

    if (ready != Socket::EventType::READ)
        return false;

    std::unique_ptr<StreamSocket> client(s.accept());

    if (!client)
        return false;

    // clear buffer
    buff.resize(BUFFER_SIZE);

    int receivedLength = client->receive(buff);
    if (receivedLength < 0) {
        std::cerr << "[Listener] Client error" << std::endl;
        client->close();
        return false;
    } else if (receivedLength < 1) {
        std::cerr << "[Listener] Client disconnected" << std::endl;
        client->close();
        return false;
    }

    client->close();
    return true;
}

// listens for data from client over UNIX socket
void listenerThread() {
    // clean up socket
    unlink(SOCK_PATH);

    // set up UNIX socket listener:
    UnixSocket s;
    s.setNonBlocking(true);

    std::cout << "[Listener] Binding to socket '" << SOCK_PATH << "'..." << std::endl;
    s.bind(SOCK_PATH);

    std::cout << "[Listener] Listening..." << std::endl;
    s.listen();

    std::vector<uint8_t> buff;

    while (!daemonExit && !daemonError) {
        try {
            std::cout << "[Listener] Waiting for data..." << std::endl;

            if (listenerWaitForData(s, buff)) {

                std::string data(buff.begin(), buff.end());

                std::cout << "[Listener] Received (" << data.size() << "): " << data << std::endl;

                CommandType c = CommandType::NONE;
                if (listenerParseCommand(data, c)) {
                    listenerSendToWorker(c);
                } else {
                    std::cerr << "[Listener] Error: \"" << data << "\": Invalid command!" << std::endl;
                }
            }
        } catch (std::exception &ex) {
            std::cerr << "[Listener] Exception: " << ex.what() << std::endl;
            // TODO: handle excptions??
            throw ex;
        }
    }

    s.close();

    listenerSendToWorker(CommandType::WAKEUP);

    // clean up socket
    unlink(SOCK_PATH);

    std::cout << "[Listener] Done" << std::endl;
}
*/
/////////////////////////////////////////////////////////////////

/*
void workerHandleCommand(CommandType command) {
    if (command == CommandType::DISCOVER) {
        upnpDiscover();
    } else if (command == CommandType::WAKEUP) {
        std::cout << "[Worker] Received WAKEUP command" << std::endl;
    }
}

bool workerWaitForCommand(CommandType &outCommand) {
    std::unique_lock<std::mutex> lock(listenerMutex);

    // Wait until data is available or timeout occurs
    if (listenerCV.wait_for(lock, std::chrono::seconds(WORKER_CV_TIMEOUT), [] { return listenerDataAvailable || daemonExit || daemonError; })) {

        if (listenerDataAvailable) {
            listenerDataAvailable = false;

            // std::cout << "[Worker] Data received from main thread!" << std::endl;
            outCommand = listenerCommand;

            return true;
        }
    }
    return false;
}

// does UPnP stuff
void workerThread() {
    while (!daemonExit && !daemonError) {
        try {
            std::cout << "[Worker] Waiting..." << std::endl;

            CommandType c = CommandType::NONE;

            if (workerWaitForCommand(c)) {
                workerHandleCommand(c);
            }
        } catch (std::exception &ex) {
            std::cerr << "[Worker] Exception: " << ex.what() << std::endl;
            // TODO: handle excptions??
            throw ex;
        }
    }

    daemonExit.store(true);
    std::cout << "[Worker] Done" << std::endl;
}

int runDaemon(const Args &args) {
    // catch SIGINT
    std::signal(SIGINT, sigint_handler);

    std::thread upnpWorkerThread(workerThread);

    listenerThread();

    upnpWorkerThread.join();

    std::cout << "Daemon exiting (" << daemonError << ")" << std::endl;
    return daemonError;
}

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
        buff.resize(BUFFER_SIZE);

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
