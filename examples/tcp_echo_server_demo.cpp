// Standalone demo: starts a TcpEchoServer, connects a loopback client to
// it, sends a message, and prints what came back.
//
//   cmake -B build && cmake --build build
//   ./build/examples/tcp_echo_server_demo

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "protocolscpp/tcp_echo_server.h"

int main() {
    using protocolscpp::TcpEchoServer;

    TcpEchoServer server(0);
    server.SetOnMessage([](const std::string& msg) { std::cout << "[server] received: " << msg << "\n"; });
    server.Start();
    std::cout << "Echo server listening on 127.0.0.1:" << server.Port() << "\n";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server.Port());
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    for (int i = 0; i < 50 && connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0; ++i) {
        usleep(10000);
    }

    std::string outgoing = "hello from the demo client";
    send(fd, outgoing.data(), outgoing.size(), 0);
    std::vector<char> buf(outgoing.size());
    std::size_t received = 0;
    while (received < buf.size()) {
        ssize_t n = recv(fd, buf.data() + received, buf.size() - received, 0);
        if (n <= 0) break;
        received += static_cast<std::size_t>(n);
    }
    close(fd);

    std::cout << "[client] sent:     " << outgoing << "\n";
    std::cout << "[client] received: " << std::string(buf.data(), received) << "\n";

    server.Stop();
    return 0;
}
