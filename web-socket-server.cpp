#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;

void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}

int main() {
    server ws_server;
    ws_server.set_message_handler(std::bind(&on_message, &ws_server, std::placeholders::_1, std::placeholders::_2));
    ws_server.init_asio();
    ws_server.listen(9002);
    ws_server.start_accept();
    ws_server.run();
}
// [130]
