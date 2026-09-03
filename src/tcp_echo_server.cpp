#include "protocolscpp/tcp_echo_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <vector>

namespace protocolscpp {

TcpEchoServer::TcpEchoServer(std::uint16_t port) : port_(port) {}

TcpEchoServer::~TcpEchoServer() { stop(); }

void TcpEchoServer::setOnMessage(std::function<void(const std::string&)> callback) {
    onMessage_ = std::move(callback);
}

void TcpEchoServer::start() {
    if (running_.exchange(true)) return;  // already running

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        running_ = false;
        throw std::runtime_error("TcpEchoServer: socket() failed: " + std::string(std::strerror(errno)));
    }

    int reuse = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        int err = errno;
        ::close(listenFd_);
        listenFd_ = -1;
        running_ = false;
        throw std::runtime_error("TcpEchoServer: bind() failed on port " + std::to_string(port_) + ": " +
                                  std::strerror(err));
    }

    // Port 0 means "OS picks a free port" -- read it back so callers can
    // discover the actual bound port via port() (used by the test suite
    // to avoid hardcoding a port number that might be in use).
    if (port_ == 0) {
        sockaddr_in bound{};
        socklen_t len = sizeof(bound);
        if (getsockname(listenFd_, reinterpret_cast<sockaddr*>(&bound), &len) == 0) {
            port_ = ntohs(bound.sin_port);
        }
    }

    if (listen(listenFd_, /*backlog=*/16) < 0) {
        int err = errno;
        ::close(listenFd_);
        listenFd_ = -1;
        running_ = false;
        throw std::runtime_error("TcpEchoServer: listen() failed: " + std::string(std::strerror(err)));
    }

    acceptThread_ = std::thread([this] { acceptLoop(); });
}

void TcpEchoServer::stop() {
    if (!running_.exchange(false)) return;

    if (listenFd_ >= 0) {
        // shutdown() unblocks a thread parked in accept() on this socket;
        // close() alone does not reliably do that on Linux.
        ::shutdown(listenFd_, SHUT_RDWR);
        ::close(listenFd_);
        listenFd_ = -1;
    }
    if (acceptThread_.joinable()) acceptThread_.join();
}

void TcpEchoServer::acceptLoop() {
    while (running_.load()) {
        int clientFd = accept(listenFd_, nullptr, nullptr);
        if (clientFd < 0) {
            // Expected once stop() shuts the listening socket down.
            if (!running_.load()) break;
            continue;
        }
        handleClient(clientFd);
    }
}

void TcpEchoServer::handleClient(int clientFd) {
    std::vector<char> buffer(4096);
    while (running_.load()) {
        ssize_t received = recv(clientFd, buffer.data(), buffer.size(), 0);
        if (received <= 0) break;  // client closed the connection, or an error

        std::string message(buffer.data(), static_cast<std::size_t>(received));
        if (onMessage_) onMessage_(message);

        std::size_t sent = 0;
        while (sent < message.size()) {
            ssize_t n = send(clientFd, message.data() + sent, message.size() - sent, 0);
            if (n <= 0) break;
            sent += static_cast<std::size_t>(n);
        }
    }
    ::close(clientFd);
}

}  // namespace protocolscpp
