#include "Daemon.hpp"

#include "TCPSocket.hpp"
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

static bool daemonExit = false;

void sigint_handler(int signum) {
    std::cout << "Caught signal '" << signum << "', interrupted..." << std::endl;
    daemonExit = true;
}

HTTP::Response makeSSDPRequest(const std::string &method, const std::string &host, int port, const std::string &path,
    const std::unordered_map<std::string, std::string> &headers, long timeout = 10) {

    std::string authority = host + ":" + std::to_string(port);
    std::cout << "[Daemon] Making SSDP request to " << authority << std::endl;

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
    // serialiseSSDPRequest(sendBuff, host, port, method, path, headers);
    HTTP::Request req(method, path, authority);
    for (const auto &i : headers) {
        req.addHeader(i.first, i.second);
    }
    req.build(sendBuff);

    // Send the SSDP multicast message
    int sentBytes = s.sendTo(host, port, sendBuff);
    if (sentBytes <= 0) {
        throw std::runtime_error("Failed to send UDP message to " + authority);
    }

    // Wait for the response
    Socket::EventType reply = s.select(timeout, false);

    if (reply != Socket::EventType::READ) {
        throw std::runtime_error("'select()' call timeout!");
    }

    std::tuple<ssize_t, std::string, int> reply_t = s.receiveFrom(replyBuff);
    if (std::get<0>(reply_t) <= 0) {
        throw std::runtime_error("No reply from " + authority);
    }

    std::cout << "[Daemon] SSDP Reply from " << std::get<1>(reply_t) << ":" << std::to_string(std::get<2>(reply_t)) << " (" << std::to_string(replyBuff.size())
              << " bytes)" << std::endl;

    // parse & return the response
    return HTTP::Response(replyBuff);
}

HTTP::Response makeHTTPRequest(const std::string &host, int port, HTTP::Request &req) {
    // connect to host:port
    TCPSocket s;

    std::vector<uint8_t> outBuff;
    std::vector<uint8_t> recvBuff;

    req.build(outBuff);

    s.setNonBlocking(true);
    s.connect(host, port);

    Socket::EventType ready = s.select(LISTENER_SELECT_TIMEOUT, true, false);
    if (ready != Socket::EventType::WRITE)
        throw std::runtime_error("Connection timed out while connecting");

    s.send(outBuff);

    ssize_t recvSize = 0;
    do {
        Socket::EventType e = s.select(LISTENER_SELECT_TIMEOUT, false, true);
        if (e == Socket::EventType::TIMEOUT)
            break;
        std::vector<uint8_t> buff;
        buff.resize(BUFFER_SIZE);
        recvSize = s.receive(buff);
        if (recvSize > 0) {
            std::copy(buff.begin(), buff.end(), std::back_inserter(recvBuff));
        }
    } while (recvSize > 0);

    // std::cout << "DEBUG: " << std::string(recvBuff.begin(), recvBuff.end()) << std::endl;

    return HTTP::Response(recvBuff);
}

StreamSocket *acceptTimeout(StreamSocket &s, long timeout) {
    Socket::EventType ready = s.select(timeout);
    if (ready != Socket::EventType::READ)
        return nullptr;
    return s.accept();
}

int runDaemon(const Args &args) {
    UNUSED(args);
    // catch SIGINT
    signal(SIGINT, sigint_handler);

    Daemon d;

    d.listen();

    int daemonError = d.run(daemonExit);

    std::cout << "[Daemon] Done, daemon exiting with code " << daemonError << std::endl;

    return daemonError;
}

/*
 * class Daemon
 */

Daemon::Daemon() {
    // clean up socket
    unlink(SOCK_PATH);
}

Daemon::~Daemon() {
    m_socket.close();

    // clean up socket
    unlink(SOCK_PATH);
}

void Daemon::listen() {

    m_socket.setNonBlocking(true);

    std::cout << "[Daemon] Binding to socket '" << SOCK_PATH << "'..." << std::endl;
    m_socket.bind(SOCK_PATH);

    std::cout << "[Daemon] Listening..." << std::endl;
    m_socket.listen();
}

int Daemon::run(bool &shouldExit) {
    bool hasError = false;
    int returnCode = 0;

    std::vector<uint8_t> buff;

    // main loop
    while (!shouldExit && !hasError) {
        std::cout << "[Daemon] Waiting for connection..." << std::endl;
        try {
            std::unique_ptr<StreamSocket> client(acceptTimeout(m_socket, LISTENER_SELECT_TIMEOUT));

            if (client) {
                // clear buffer
                buff.resize(BUFFER_SIZE);

                try {
                    int receivedLength = client->receive(buff);
                    if (receivedLength < 0) {
                        throw std::runtime_error("Client error");
                    } else if (receivedLength < 1) {
                        throw std::runtime_error("Client disconnected");
                    }
                } catch (const std::exception &ex) {
                    client->close();
                    std::cerr << "[Daemon] Client exception: " << ex.what() << std::endl;
                    continue;
                }
                client->close();

                std::string data(buff.begin(), buff.end());
                std::cout << "[Daemon] Received (" << data.size() << "): " << data << std::endl;

                runCommand(data);

                // client->close();
            }
        } catch (const std::system_error &ex) {
            if (ex.code().value() != 4) {
                returnCode = 2;
                throw;
            }
        } catch (const std::exception &ex) {
            std::cerr << "[Daemon] Exception: " << ex.what() << std::endl;
            // TODO: handle excptions??
            returnCode = 1;
            throw;
        }
    }
    m_socket.close();

    return returnCode;
}

int Daemon::runCommand(const std::string &command) {
    try {
        if (command == "discover") {
            discoverIGD();
        } else if (command == "servicedescription") {
            getIGDServiceDescription();
        } else {
            throw std::runtime_error("Invalid command");
        }
    } catch (std::exception &ex) {
        std::cerr << "Running command '" << command << "' failed: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}

void Daemon::discoverIGD() {
    std::cout << "[Daemon] Discovering UPnP IGD device..." << std::endl;

    std::unordered_map<std::string, std::string> headers({ { "ST", "ssdp:all" }, { "Man", "\"ssdp:discover\"" }, { "MX", "3" } });

    auto response = makeSSDPRequest("M-SEARCH", "239.255.255.250", 1900, "*", headers);

    std::cout << "[Daemon] SSDP Response status code: " << response.getStatusCode() << std::endl;

    if (!response.ok())
        throw std::runtime_error("Received non-ok response code " + std::to_string(response.getStatusCode()));

    auto loc = response.getHeader("location");
    if (!loc.has_value())
        throw std::runtime_error("Missing 'location' header in response");

    std::cout << "[Daemon] UPnP device location: '" << loc.value() << "'" << std::endl;

    m_IGDLocation = HTTP::parseUrl(loc.value());

    std::cout << "[Daemon] Discover done" << std::endl;
}

void Daemon::getIGDServiceDescription() {
    std::cout << "[Daemon] Discovering UPnP IGD services..." << std::endl;

    if (!m_IGDLocation.has_value())
        throw std::runtime_error("Cannot get service description: IGD location not set!");

    HTTP::Request req("GET", m_IGDLocation->path, m_IGDLocation->host + ":" + std::to_string(m_IGDLocation->port));
    auto res = makeHTTPRequest(m_IGDLocation->host, m_IGDLocation->port, req);

    std::cout << "[Daemon] response status code: " << res.getStatusCode() << std::endl;

    if (!res.ok())
        throw std::runtime_error("Received non-ok response code " + std::to_string(res.getStatusCode()));

    std::cout << std::string(res.getBody().begin(), res.getBody().end()) << std::endl;
    /*
    auto loc = res.getHeader("location");
    if (!loc.has_value())
        throw std::runtime_error("Missing 'location' header in res");

    std::cout << "[Daemon] UPnP device location: '" << loc.value() << "'" << std::endl;

    m_IGDLocation = loc.value();
    */

    std::cout << "[Daemon] Discover done" << std::endl;
}
