#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace protocolscpp {

// A minimal, dependency-free TCP echo server over POSIX sockets --
// accepts connections on a background thread and echoes back whatever
// each client sends, one connection at a time, until stop() is called.
//
// This is intentionally small (no epoll/select multiplexing, one client
// at a time) so it stays understandable as a reference for the
// socket()/bind()/listen()/accept() lifecycle; TcpEchoServerTest exercises
// it against a real loopback client with no mocks, external services, or
// third-party dependencies required.
class TcpEchoServer {
public:
    explicit TcpEchoServer(std::uint16_t port);
    ~TcpEchoServer();

    TcpEchoServer(const TcpEchoServer&) = delete;
    TcpEchoServer& operator=(const TcpEchoServer&) = delete;

    // Binds and starts accepting connections on a background thread.
    // Throws std::runtime_error if the socket can't be created/bound.
    void start();
    void stop();

    // Optional hook invoked (from the background thread) with each
    // received line, mainly so tests can observe what the server saw.
    void setOnMessage(std::function<void(const std::string&)> callback);

    std::uint16_t port() const { return port_; }
    bool running() const { return running_.load(); }

private:
    void acceptLoop();
    void handleClient(int clientFd);

    std::uint16_t port_;
    int listenFd_ = -1;
    std::atomic<bool> running_{false};
    std::thread acceptThread_;
    std::function<void(const std::string&)> onMessage_;
};

}  // namespace protocolscpp
