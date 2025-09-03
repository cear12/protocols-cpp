// C++ Native Messaging Host: Communicates over stdin/stdout with Chrome
// Reads message, writes reply as 32-bit length-prefixed JSON
// Manifest.json, registry (Win), and host binary required
#include <iostream>
#include <string>

#include <json/json.h> // use any json lib

int main() {
    while (true) {
        uint32_t len = 0;
        std::cin.read(reinterpret_cast<char*>(&len), 4);
        std::string s(len, '\0');
        std::cin.read(&s[0], len);

        Json::Value msg;
        Json::Reader().parse(s, msg);
        if (msg["text"].asString() == "#STOP#") break;

        Json::Value reply;
        reply["response"] = "pong";
        std::string out = reply.toStyledString();
        uint32_t olen = out.size();
        std::cout.write(reinterpret_cast<const char*>(&olen), 4);
        std::cout << out << std::flush;
    }
}
