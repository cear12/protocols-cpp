#include "easywsclient.hpp"

using easywsclient::WebSocket;

int main() {
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:9002");
    ws->send("Hello from easywsclient!");
    ws->poll();
    ws->close();
}
// [130]
