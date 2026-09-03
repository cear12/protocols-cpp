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
    void Start();
    void Stop();

    // Optional hook invoked (from the background thread) with each
    // received line, mainly so tests can observe what the server saw.
    void SetOnMessage(std::function<void(const std::string&)> callback);

    std::uint16_t Port() const { return port_; }
    bool Running() const { return running_.load(); }

private:
    void AcceptLoop();
    void HandleClient(int client_fd);

    std::uint16_t port_;
    int listen_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::function<void(const std::string&)> on_message_;
};

}  // namespace protocolscpp
