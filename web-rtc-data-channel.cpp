// Simplified WebRTC data channel example using libdatachannel
#include <rtc/rtc.hpp>
class WebRTCDataChannel {
    std::shared_ptr<rtc::PeerConnection> pc;
    std::shared_ptr<rtc::DataChannel> dc;
    
public:
    WebRTCDataChannel() {
        rtc::Configuration config;
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        pc = std::make_shared<rtc::PeerConnection>(config);
        
        pc->onStateChange([](rtc::PeerConnection::State state) {
            std::cout << "State: " << static_cast<int>(state) << std::endl;
        });
        
        dc = pc->createDataChannel("test");
        dc->onOpen([]() {
            std::cout << "DataChannel opened" << std::endl;
        });
        
        dc->onMessage([](rtc::message_variant data) {
            if (std::holds_alternative<std::string>(data)) {
                std::cout << "Received: " << std::get<std::string>(data) << std::endl;
            }
        });
    }
    
    void sendMessage(const std::string& message) {
        if (dc && dc->isOpen()) {
            dc->send(message);
        }
    }
    
    std::string createOffer() {
        auto offer = pc->createOffer();
        pc->setLocalDescription(offer);
        return offer;
    }
};
