// SMTP client wrapping libcurl's mail-sending support (RFC 5321-ish, via
// libcurl's SMTP protocol handler).
//
// Build requirement: libcurl (Ubuntu/Debian: `apt-get install
// libcurl4-openssl-dev`). Wired into CMakeLists.txt behind
// `find_package(CURL)`; not compiled as part of this environment's own
// verification pass -- see README.md.
#include <curl/curl.h>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace protocolscpp {

// Feeds libcurl's SMTP upload the message body one line at a time via
// CURLOPT_READFUNCTION -- libcurl calls back into readCallback() until it
// returns 0, which is how it discovers the message is complete.
class SmtpEmailClient {
 public:
  SmtpEmailClient() : curl_(curl_easy_init()) {
    if (curl_ == nullptr)
      throw std::runtime_error("SmtpEmailClient: curl_easy_init() failed");
  }

  ~SmtpEmailClient() {
    if (curl_ != nullptr) curl_easy_cleanup(curl_);
  }

  SmtpEmailClient(const SmtpEmailClient&) = delete;
  SmtpEmailClient& operator=(const SmtpEmailClient&) = delete;

  // `server` is an smtp:// or smtps:// URL, e.g.
  // "smtps://smtp.example.com:465".
  void sendEmail(const std::string& server, const std::string& from,
                 const std::string& to, const std::string& subject,
                 const std::string& body) {
    MessageState state;
    state.lines = {
        "To: " + to + "\r\n",
        "From: " + from + "\r\n",
        "Subject: " + subject + "\r\n",
        "\r\n",
        body + "\r\n",
    };

    curl_easy_setopt(curl_, CURLOPT_URL, server.c_str());
    curl_easy_setopt(curl_, CURLOPT_MAIL_FROM, from.c_str());

    curl_slist* recipients = curl_slist_append(nullptr, to.c_str());
    curl_easy_setopt(curl_, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl_, CURLOPT_READFUNCTION,
                     &SmtpEmailClient::readCallback);
    curl_easy_setopt(curl_, CURLOPT_READDATA, &state);
    curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);

    CURLcode res = curl_easy_perform(curl_);
    curl_slist_free_all(recipients);

    if (res != CURLE_OK) {
      throw std::runtime_error(
          std::string("SmtpEmailClient::sendEmail failed: ") +
          curl_easy_strerror(res));
    }
  }

 private:
  struct MessageState {
    std::vector<std::string> lines;
    std::size_t currentLine = 0;
  };

  static std::size_t readCallback(void* ptr, std::size_t size,
                                  std::size_t nmemb, void* userp) {
    auto* state = static_cast<MessageState*>(userp);
    if (state->currentLine >= state->lines.size())
      return 0;  // signals "no more data"

    const std::string& line = state->lines[state->currentLine++];
    std::size_t len = std::min(line.size(), size * nmemb);
    std::memcpy(ptr, line.data(), len);
    return len;
  }

  CURL* curl_;
};

}  // namespace protocolscpp

#ifdef PROTOCOLSCPP_BUILD_STANDALONE_DEMO
#include <iostream>
int main() {
  protocolscpp::SmtpEmailClient client;
  try {
    client.sendEmail("smtps://smtp.example.com:465", "sender@example.com",
                     "recipient@example.com", "Test subject", "Test body");
    std::cout << "Email sent\n";
  } catch (const std::exception& e) {
    std::cerr << "Send failed (expected without a real SMTP server): "
              << e.what() << "\n";
  }
}
#endif
