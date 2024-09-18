#pragma once

#include <string>

class CurlHelper
{
public:
    CurlHelper();

    struct Response {
        std::string body;
        long http_code;
    };

    Response GET(const std::string& url);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    static bool curl_initialized;
};
