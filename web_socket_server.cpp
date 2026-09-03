// WebSocket echo server using websocketpp
// (https://github.com/zaphoyd/websocketpp), a header-only library built on
// Boost.Asio (or standalone Asio).
//
// Build requirement: websocketpp + Boost or standalone Asio
// (Ubuntu/Debian: `apt-get install libwebsocketpp-dev libboost-dev`).
// Wired into CMakeLists.txt behind `find_package(websocketpp)`; not
// compiled as part of this environment's own verification pass -- see
// README.md.
#include <functional>
#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace protocolscpp {

using WebSocketServer = websocketpp::server<websocketpp::config::asio>;

// Echoes every received message back to the same connection, preserving
// the original opcode (so binary frames stay binary, text stays text).
class WebSocketEchoServer {
 public:
  explicit WebSocketEchoServer(std::uint16_t port) : port_(port) {
    server_.init_asio();
    server_.set_message_handler(
        [this](websocketpp::connection_hdl hdl,
               WebSocketServer::message_ptr msg) { onMessage(hdl, msg); });
  }

  void run() {
    server_.listen(port_);
    server_.start_accept();
    server_.run();  // blocks until stop_listening()/stop() is called
  }

  void stop() { server_.stop(); }

 private:
  void onMessage(websocketpp::connection_hdl hdl,
                 WebSocketServer::message_ptr msg) {
    server_.send(hdl, msg->get_payload(), msg->get_opcode());
  }

  WebSocketServer server_;
  std::uint16_t port_;
};

}  // namespace protocolscpp

#ifdef PROTOCOLSCPP_BUILD_STANDALONE_DEMO
int main() {
  protocolscpp::WebSocketEchoServer server(9002);
  std::cout << "WebSocket echo server listening on ws://localhost:9002\n";
  server.run();
}
#endif
