#include "Daemon.hpp"

#include "UDPSocket.hpp"
#include "UnixSocket.hpp"
// #include <atomic>
#include <csignal>
#include <iostream>
// #include <thread>
#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define LISTENER_SELECT_TIMEOUT 15
#define WORKER_CV_TIMEOUT 15

#define BUFFER_SIZE 2048

static bool daemonExit(false);
int daemonError(0);

void sigint_handler(int signum) {
    std::cout << "Caught signal '" << signum << "', interrupted..." << std::endl;
    daemonExit = true;
}

int vtoi(const std::vector<uint8_t> &vec) {
    int value = 0;
    // convert to integer:
    for (auto code : vec) {
        value = value * 10 + (code - '0');
    }
    return value;
}

void serialiseSSDPRequest(std::vector<uint8_t> &buff, const std::string &ip, int port, const std::string &method, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers) {

    /*
        M-SEARCH * HTTP/1.1\r\n
        Host: 239.255.255.250:1900\r\n
        ST: ssdp:all\r\n
        Man: \"ssdp:discover\"\r\n
        MX: 3\r\n\r\n
    */

    // construct the message:
    std::stringstream ss;
    ss << method << " " << path << " "
       << "HTTP/"
       << "1.1"
       << "\r\n";
    ss << "Host"
       << ": " << ip << ":" << port << "\r\n";
    for (const auto &i : headers) {
        ss << i.first << ": " << i.second << "\r\n";
    }
    ss << "\r\n";

    // sendBuff.insert(sendBuff.end(), method.begin(), method.end());
    // sendBuff.insert(sendBuff.end(), ss.begin(), ss.end());

    buff.clear();
    std::copy(std::istreambuf_iterator<char>(ss), std::istreambuf_iterator<char>(), std::back_inserter(buff));
}

std::pair<int, std::unordered_map<std::string, std::string>> parseSSDPResponse(std::vector<uint8_t> &buff) {

    /*
        HTTP/1.1 200 OK
        Server: Custom/1.0 UPnP/1.0 Proc/Ver
        EXT:
        Location: http://192.168.0.1:5431/dyndev/uuid:3c9ec793-3e70-703e-93c7-9e3c9e93700000
        Cache-Control:max-age=1800
        ST:upnp:rootdevice
        USN:uuid:3c9ec793-3e70-703e-93c7-9e3c9e93700000::upnp:rootdevice
    */

    std::unordered_map<std::string, std::string> headers;

    // parse response
    auto iter = buff.begin();
    auto end = buff.end();

    std::vector<uint8_t> terminator{ '\r', '\n' };
    std::vector<uint8_t> headerSep{ ':' };

    bool isStatusLine = true;

    int responseCode = -1;
    std::unordered_map<std::string, std::string> receivedHeaders;

    while (iter < end - 2) {
        auto chunkStart = iter;
        auto chunkEnd = std::search(iter, end, terminator.begin(), terminator.end());

        std::string line(chunkStart, chunkEnd);
        if (isStatusLine) {
            std::cout << "Status Line: " << line << std::endl;
            isStatusLine = false;

            int statusParts = 0;
            auto partStart = chunkStart;
            while (statusParts < 3 && partStart < chunkEnd) {
                auto partEnd = std::find(partStart, chunkEnd, ' ');

                if (statusParts == 1) {
                    std::vector<uint8_t> subPart(partStart, partEnd);
                    responseCode = vtoi(subPart);
                }
                ++statusParts;
                partStart = std::next(partEnd, 1);
            }
            if (statusParts != 3 || partStart < chunkEnd)
                throw std::runtime_error("Failed to parse HTTP status line");

        } else {

            auto keyEnd = std::find(chunkStart, chunkEnd, ':');
            if (keyEnd >= chunkEnd) {
                throw std::runtime_error("Failed to parse HTTP header - ':' not found!");
            }

            auto valueStart = keyEnd + 1;
            if (*(valueStart) == ' ') {
                ++valueStart;
            }

            std::string key(chunkStart, keyEnd);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            receivedHeaders.insert(std::make_pair(key, std::string(valueStart, chunkEnd)));
        }
        // Advance the iterator to the next chunk
        iter = std::next(chunkEnd, 2);
    }

    return std::pair<int, std::unordered_map<std::string, std::string>>(responseCode, receivedHeaders);
}

std::pair<int, std::unordered_map<std::string, std::string>> makeSSDPRequest(const std::string &ip, int port, const std::string &method,
    const std::string &path, const std::unordered_map<std::string, std::string> &headers, long timeout) {
    std::cout << "[Listener] Making SSDP request to " << ip << ":" << port << std::endl;

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
    serialiseSSDPRequest(sendBuff, ip, port, method, path, headers);

    // Send the SSDP multicast message
    int sentBytes = s.sendTo(ip, port, sendBuff);
    if (sentBytes <= 0) {
        throw std::runtime_error("Failed to send UDP message to " + ip + ":" + std::to_string(port));
    }

    // Wait for the response
    Socket::EventType reply = s.select(timeout, false);

    if (reply != Socket::EventType::READ) {
        throw std::runtime_error("'select()' call timeout!");
    }

    std::tuple<ssize_t, std::string, int> reply_t = s.receiveFrom(replyBuff);
    if (std::get<0>(reply_t) <= 0) {
        throw std::runtime_error("No reply from " + ip + ":" + std::to_string(port));
    }

    std::cout << "[Listener] SSDP Reply from " << std::get<1>(reply_t) << ":" << std::to_string(std::get<2>(reply_t)) << " ("
              << std::to_string(replyBuff.size()) << " bytes)" << std::endl;

    // parse the response
    return parseSSDPResponse(replyBuff);
}

void upnpDiscover() {

    std::cout << "[Listener] Discovering UPnP devices..." << std::endl;

    std::unordered_map<std::string, std::string> headers({ { "ST", "ssdp:all" }, { "Man", "\"ssdp:discover\"" }, { "MX", "3" } });

    auto response = makeSSDPRequest("239.255.255.250", 1900, "M-SEARCH", "*", headers, 10);

    std::cout << "[Listener] SSDP Response code: " << response.first << std::endl;

    if (response.first > 200 || response.first > 399)
        throw std::runtime_error("Received non-ok response code " + std::to_string(response.first));

    std::cout << "[Listener] UPnP device location: '" << response.second["location"] << "'" << std::endl;

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
