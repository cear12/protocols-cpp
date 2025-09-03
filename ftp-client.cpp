#include <curl/curl.h>
class FTPClient {
    CURL* curl;
    std::string response;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        s->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
public:
    FTPClient() { curl = curl_easy_init(); }
    ~FTPClient() { curl_easy_cleanup(curl); }
    
    std::string downloadFile(const std::string& ftp_url, const std::string& username = "", const std::string& password = "") {
        response.clear();
        curl_easy_setopt(curl, CURLOPT_URL, ftp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        if (!username.empty()) {
            curl_easy_setopt(curl, CURLOPT_USERPWD, (username + ":" + password).c_str());
        }
        CURLcode res = curl_easy_perform(curl);
        return response;
    }
    
    bool uploadFile(const std::string& ftp_url, const std::string& local_file) {
        FILE* file = fopen(local_file.c_str(), "rb");
        if (!file) return false;
        
        curl_easy_setopt(curl, CURLOPT_URL, ftp_url.c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        
        CURLcode res = curl_easy_perform(curl);
        fclose(file);
        return res == CURLE_OK;
    }
};
