#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#include "catch.hpp"
#include "protocolscpp/tcp_echo_server.h"

using namespace protocolscpp;

namespace {
// A tiny synchronous loopback client -- just enough POSIX socket code to
// exercise TcpEchoServer without pulling in a second library.
std::string echoOnce(std::uint16_t port, const std::string& message) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // The server's accept thread starts asynchronously; retry briefly
    // rather than requiring the caller to sleep an arbitrary amount.
    int rv = -1;
    for (int attempt = 0; attempt < 50 && rv != 0; ++attempt) {
        rv = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (rv != 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(rv == 0);

    send(fd, message.data(), message.size(), 0);
    std::vector<char> buf(message.size());
    std::size_t received = 0;
    while (received < buf.size()) {
        ssize_t n = recv(fd, buf.data() + received, buf.size() - received, 0);
        if (n <= 0) break;
        received += static_cast<std::size_t>(n);
    }
    close(fd);
    return std::string(buf.data(), received);
}
}  // namespace

TEST_CASE("TcpEchoServer echoes back exactly what a client sends", "[tcp_echo_server]") {
    TcpEchoServer server(0);  // port 0 = let the OS choose a free port
    server.start();
    REQUIRE(server.running());

    auto reply = echoOnce(server.port(), "hello, echo server");
    REQUIRE(reply == "hello, echo server");

    server.stop();
    REQUIRE_FALSE(server.running());
}

TEST_CASE("TcpEchoServer invokes the onMessage callback with what it received", "[tcp_echo_server]") {
    TcpEchoServer server(0);
    std::atomic<int> callCount{0};
    std::string lastMessage;
    server.setOnMessage([&](const std::string& msg) {
        lastMessage = msg;
        callCount++;
    });
    server.start();

    echoOnce(server.port(), "ping");
    server.stop();

    REQUIRE(callCount.load() == 1);
    REQUIRE(lastMessage == "ping");
}

TEST_CASE("TcpEchoServer::start is idempotent and stop can be called twice safely", "[tcp_echo_server]") {
    TcpEchoServer server(0);
    server.start();
    server.start();  // no-op, must not throw or open a second listener
    REQUIRE(server.running());

    server.stop();
    server.stop();  // no-op
    REQUIRE_FALSE(server.running());
}

TEST_CASE("TcpEchoServer handles multiple sequential connections", "[tcp_echo_server]") {
    TcpEchoServer server(0);
    server.start();

    REQUIRE(echoOnce(server.port(), "first") == "first");
    REQUIRE(echoOnce(server.port(), "second") == "second");
    REQUIRE(echoOnce(server.port(), "third") == "third");

    server.stop();
}
