#include "http_client.h"
#include <curl/curl.h>
#include <string>
#include <iostream>

bool HttpClient::curl_initialized = false;

// Constructor
HttpClient::HttpClient() {
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_initialized = true;
    }
}

// Write callback function to store the response body
size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* response_body = static_cast<std::string*>(userp);
    response_body->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Perform an HTTP GET request and return the response object
HttpClient::Response HttpClient::GET(const std::string& url) {
    CURL* curl;
    CURLcode res;
    Response response;

    curl = curl_easy_init();
    if (curl) {
        std::string response_body;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "KX-Trainer-Free/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.http_code);
            response.body = response_body;
        }

        curl_easy_cleanup(curl);
    }

    return response;
}
