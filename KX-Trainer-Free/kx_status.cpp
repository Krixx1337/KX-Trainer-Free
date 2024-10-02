#include "nlohmann/json.hpp"
#include "http_client.h"
#include "curl/curl.h"
#include <iostream>

#include "kx_status.h"

#include "constants.h"

using json = nlohmann::json;

bool KXStatus::CheckStatus()
{
#ifdef _DEBUG
    return true;
#endif

    bool verified = false;

    HttpClient curlHelper;
    HttpClient::Response response = curlHelper.GET(Constants::API_URL);

    if (response.http_code != 200) {
        std::cout << "An error occurred while checking the status! HTTP code: " << response.http_code << std::endl;
    }
    else {
        // Process the response body to verify status
        if (ReadStatusJson(response.body)) {
            verified = true;
        }
    }

    return verified;
}


bool KXStatus::ReadStatusJson(std::string readBuffer)
{
    Status status;
    json j = json::parse(readBuffer);

    if (!j.contains("status") || !j.contains("version") || !j.contains("message") || !j.contains("download"))
    {
	    std::cout << "An error occurred while checking the status! (json)\n";
	    return false;
    }

    j.at("status").get_to(status.status);
    j.at("version").get_to(status.version);
    j.at("message").get_to(status.message);
    j.at("download").get_to(status.download);

    if (status.status == 1)
    {
        if (status.version <= Constants::APP_VERSIONNR)
        {
            std::cout << "You are using latest " << Constants::APP_NAME << " version\n";
            if (!status.message.empty())
            {
                std::cout << status.message << std::endl;
            }

            Sleep(1000);
            return true;
        }

        std::cout << "You are using an outdated version of " << Constants::APP_NAME << ", download page with latest version will open now!\n";
        ShellExecuteA(NULL, "open", status.download.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else
    {
        std::cout << Constants::APP_NAME << " is currently disabled, please try again later!\n";
    }

    return false;
}
