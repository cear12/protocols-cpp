#include <curl/curl.h>
struct EmailData {
    std::vector<std::string> lines;
    size_t current_line = 0;
};

// Callback для отправки email через libcurl
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    EmailData *data = static_cast<EmailData*>(userp);
    if (data->current_line < data->lines.size()) {
        std::string &line = data->lines[data->current_line++];
        size_t len = std::min(line.length(), size * nmemb);
        memcpy(ptr, line.c_str(), len);
        return len;
    }
    return 0;
}

class SMTPClient {
    CURL* curl;
public:
    SMTPClient() { curl = curl_easy_init(); }
    ~SMTPClient() { curl_easy_cleanup(curl); }
    
    void sendEmail(const std::string& server, const std::string& from,
                   const std::string& to, const std::string& subject, 
                   const std::string& body) {
        EmailData email_data;
        email_data.lines = {
            "To: " + to + "\r\n",
            "From: " + from + "\r\n", 
            "Subject: " + subject + "\r\n",
            "\r\n",
            body + "\r\n"
        };
        
        curl_easy_setopt(curl, CURLOPT_URL, server.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
        struct curl_slist* recipients = curl_slist_append(nullptr, to.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &email_data);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(recipients);
    }
};
