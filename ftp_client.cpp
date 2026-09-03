// FTP client wrapping libcurl's easy interface.
//
// Build requirement: libcurl (Ubuntu/Debian: `apt-get install
// libcurl4-openssl-dev`). This file is wired into CMakeLists.txt behind
// `find_package(CURL)` -- it is NOT compiled as part of this environment's own
// verification pass (no libcurl dev headers are installed here); see README.md.
#include <curl/curl.h>

#include <cstdio>
#include <stdexcept>
#include <string>

namespace protocolscpp {

// Thin RAII wrapper around a CURL easy handle configured for FTP
// upload/download. One instance is not safe to use from multiple threads
// concurrently (same restriction as the underlying CURL* handle).
class FtpClient {
 public:
  FtpClient() : curl_(curl_easy_init()) {
    if (curl_ == nullptr)
      throw std::runtime_error("FtpClient: curl_easy_init() failed");
  }

  ~FtpClient() {
    if (curl_ != nullptr) curl_easy_cleanup(curl_);
  }

  FtpClient(const FtpClient&) = delete;
  FtpClient& operator=(const FtpClient&) = delete;

  // Downloads ftp_url and returns its contents. Throws std::runtime_error
  // on any libcurl failure (auth, connection refused, etc.).
  std::string downloadFile(const std::string& ftpUrl,
                           const std::string& username = "",
                           const std::string& password = "") {
    std::string response;
    curl_easy_setopt(curl_, CURLOPT_URL, ftpUrl.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &FtpClient::writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    if (!username.empty()) {
      std::string userpwd = username + ":" + password;
      curl_easy_setopt(curl_, CURLOPT_USERPWD, userpwd.c_str());
    }

    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
      throw std::runtime_error(std::string("FtpClient::downloadFile failed: ") +
                               curl_easy_strerror(res));
    }
    return response;
  }

  // Uploads localFile's contents to ftpUrl. Returns false (rather than
  // throwing) if the local file can't be opened, since "file not found"
  // is an expected, recoverable condition for callers to check.
  bool uploadFile(const std::string& ftpUrl, const std::string& localFile) {
    FILE* file = std::fopen(localFile.c_str(), "rb");
    if (file == nullptr) return false;

    curl_easy_setopt(curl_, CURLOPT_URL, ftpUrl.c_str());
    curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl_, CURLOPT_READDATA, file);

    CURLcode res = curl_easy_perform(curl_);
    std::fclose(file);
    return res == CURLE_OK;
  }

 private:
  static std::size_t writeCallback(void* contents, std::size_t size,
                                   std::size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
  }

  CURL* curl_;
};

}  // namespace protocolscpp

#ifdef PROTOCOLSCPP_BUILD_STANDALONE_DEMO
#include <iostream>
int main() {
  protocolscpp::FtpClient client;
  try {
    // Public, well-known anonymous FTP test server.
    auto contents = client.downloadFile("ftp://ftp.gnu.org/gnu/MISSING");
    std::cout << "Downloaded " << contents.size() << " bytes\n";
  } catch (const std::exception& e) {
    std::cerr << "Download failed (expected without network access): "
              << e.what() << "\n";
  }
}
#endif
