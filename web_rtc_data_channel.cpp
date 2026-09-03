// WebRTC data channel wrapper using libdatachannel
// (https://github.com/paullouisageneau/libdatachannel).
//
// Build requirement: libdatachannel is not in standard apt repositories;
// CMakeLists.txt fetches it via FetchContent when
// PROTOCOLSCPP_ENABLE_WEBRTC=ON (off by default). See README.md.
#include <iostream>
#include <memory>
#include <rtc/rtc.hpp>
#include <stdexcept>
#include <string>
#include <variant>

namespace protocolscpp {

// Opens a peer connection with a single data channel, using Google's
// public STUN server for NAT traversal. This is the "offering" side of a
// WebRTC handshake -- createOffer()'s result still needs to be exchanged
// with a remote peer through some signaling channel (WebSocket, etc.),
// which is intentionally outside this class's scope.
class WebRtcDataChannel {
 public:
  WebRtcDataChannel() {
    rtc::Configuration config;
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    peerConnection_ = std::make_shared<rtc::PeerConnection>(config);

    peerConnection_->onStateChange([](rtc::PeerConnection::State state) {
      std::cout << "PeerConnection state: " << static_cast<int>(state) << "\n";
    });

    dataChannel_ = peerConnection_->createDataChannel("data");
    dataChannel_->onOpen([] { std::cout << "DataChannel opened\n"; });
    dataChannel_->onMessage([](rtc::message_variant data) {
      if (std::holds_alternative<std::string>(data)) {
        std::cout << "Received: " << std::get<std::string>(data) << "\n";
      }
    });
  }

  void sendMessage(const std::string& message) {
    if (!dataChannel_ || !dataChannel_->isOpen()) {
      throw std::runtime_error(
          "WebRtcDataChannel::sendMessage: data channel is not open");
    }
    dataChannel_->send(message);
  }

  // Returns the local SDP offer as a string, to be sent to the remote
  // peer through whatever signaling mechanism the application uses.
  std::string createOffer() {
    auto offer = peerConnection_->localDescription();
    return offer.has_value() ? std::string(offer.value()) : std::string{};
  }

 private:
  std::shared_ptr<rtc::PeerConnection> peerConnection_;
  std::shared_ptr<rtc::DataChannel> dataChannel_;
};

}  // namespace protocolscpp

#ifdef PROTOCOLSCPP_BUILD_STANDALONE_DEMO
int main() {
  protocolscpp::WebRtcDataChannel channel;
  std::cout << "Local offer:\n" << channel.createOffer() << "\n";
  // A full demo needs a second peer + signaling channel to exchange
  // offer/answer SDP; that exchange is application-specific and left out.
}
#endif
