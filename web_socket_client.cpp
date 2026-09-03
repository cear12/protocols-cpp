// WebSocket client using the vendored easywsclient library
// (third_party/easywsclient/, MIT licensed -- see COPYING there).
// Unlike the other files in this repo, this one has zero un-vendored
// third-party dependencies and IS built + verified by this repo's own
// CMake/CI configuration (see CMakeLists.txt, ci.yml).
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "easywsclient.hpp"

namespace protocolscpp {

// RAII wrapper: connects in the constructor (throwing if the connection
// can't be established), disconnects in the destructor.
class WebSocketClient {
 public:
  explicit WebSocketClient(const std::string& url)
      : socket_(easywsclient::WebSocket::from_url(url)) {
    if (!socket_) {
      throw std::runtime_error("WebSocketClient: failed to connect to " + url);
    }
  }

  ~WebSocketClient() {
    if (socket_) socket_->close();
  }

  WebSocketClient(const WebSocketClient&) = delete;
  WebSocketClient& operator=(const WebSocketClient&) = delete;

  void Send(const std::string& message) { socket_->send(message); }

  // Pumps the connection once: processes any pending I/O and invokes
  // `onMessage` for each complete message received during this pump.
  // Callers typically loop this (with their own pacing) for as long as
  // the connection should stay open.
  template <class OnMessage>
  void Poll(OnMessage on_message, int timeout_millis = 0) {
    socket_->poll(timeout_millis);
    socket_->dispatch(on_message);
  }

  bool IsOpen() const {
    return socket_ && socket_->getReadyState() == easywsclient::WebSocket::OPEN;
  }

 private:
  std::unique_ptr<easywsclient::WebSocket> socket_;
};

}  // namespace protocolscpp

#ifdef PROTOCOLSCPP_BUILD_STANDALONE_DEMO
int main() {
  try {
    protocolscpp::WebSocketClient client("ws://localhost:9002");
    client.Send("Hello from protocols-cpp!");
    client.Poll([](const std::string& message) {
      std::cout << "Received: " << message << "\n";
    });
  } catch (const std::exception& e) {
    std::cerr
        << "Connection failed (expected without a running WebSocket server): "
        << e.what() << "\n";
    return 1;
  }
}
#endif
