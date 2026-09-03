// Chrome/Firefox Native Messaging host: communicates with the browser
// extension over stdin/stdout using the native-messaging framing (a
// 32-bit little-endian length prefix followed by that many bytes of JSON).
// See: https://developer.chrome.com/docs/apps/nativeMessaging/
//
// Build requirement: jsoncpp (Ubuntu/Debian: `apt-get install libjsoncpp-dev`).
// Also needs a browser-side manifest.json (registered via the Windows
// registry or a manifest directory on Linux/macOS) pointing at this
// binary -- that plumbing is outside this file's scope.
#include <json/json.h>

#include <iostream>
#include <optional>
#include <string>

namespace protocolscpp {

// Reads one native-messaging frame from stdin. Returns std::nullopt on
// EOF/short read (the browser closed the pipe), so main()'s loop can exit
// cleanly instead of spinning on garbage.
std::optional<Json::Value> readNativeMessage(std::istream& in) {
    std::uint32_t length = 0;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (!in || in.gcount() != sizeof(length)) return std::nullopt;

    std::string payload(length, '\0');
    in.read(payload.data(), length);
    if (!in || static_cast<std::uint32_t>(in.gcount()) != length) return std::nullopt;

    Json::Value message;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(payload);
    if (!Json::parseFromStream(builder, stream, &message, &errors)) return std::nullopt;
    return message;
}

void writeNativeMessage(std::ostream& out, const Json::Value& message) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // compact, one line -- native messaging doesn't need pretty-printing
    std::string serialized = Json::writeString(builder, message);

    std::uint32_t length = static_cast<std::uint32_t>(serialized.size());
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
    out.flush();
}

}  // namespace protocolscpp

int main() {
    while (auto message = protocolscpp::readNativeMessage(std::cin)) {
        if ((*message)["text"].asString() == "#STOP#") break;

        Json::Value reply;
        reply["response"] = "pong";
        protocolscpp::writeNativeMessage(std::cout, reply);
    }
    return 0;
}
