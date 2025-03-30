#include "kx_status.h"
#include "nlohmann/json.hpp"
#include "http_client.h"
#include "constants.h"
#include "status_ui.h"
#include <windows.h>
#include <string>

using json = nlohmann::json;

bool KXStatus::CheckStatus()
{
#ifdef _DEBUG
    // Bypass check in debug mode for easier development
    StatusUI::AddMessage("INFO: Skipping API status check in DEBUG mode.");
    return true;
#endif

    bool verified = false;
    HttpClient curlHelper;

    try {
        HttpClient::Response response = curlHelper.GET(Constants::API_URL);

        if (response.http_code != 200) {
            StatusUI::AddMessage("ERROR: Failed to connect to status API. HTTP code: " + std::to_string(response.http_code));
        }
        else {
            // Process the response body. ReadStatusJson will log specific errors if any.
            if (ReadStatusJson(response.body)) {
                verified = true;
            }
        }
    }
    catch (const std::exception& e) {
        StatusUI::AddMessage("ERROR: Exception during HTTP request: " + std::string(e.what()));
    }
    catch (...) {
        StatusUI::AddMessage("ERROR: Unknown exception during HTTP request.");
    }

    // Note: A specific error should have been logged by ReadStatusJson or the HTTP catch blocks if verification failed.
    return verified;
}


bool KXStatus::ReadStatusJson(const std::string& json_data)
{
    try {
        Status status;
        json j = json::parse(json_data);

        // Validate JSON structure
        if (!j.contains("status") || !j.contains("version") || !j.contains("message") || !j.contains("download"))
        {
            StatusUI::AddMessage("ERROR: Invalid status API response format (missing fields).");
            return false;
        }

        // Extract data
        j.at("status").get_to(status.status);
        j.at("version").get_to(status.version);
        j.at("message").get_to(status.message);
        j.at("download").get_to(status.download);

        // Check application status field
        if (status.status == 1) // 1 = OK/Enabled
        {
            // Check application version
            if (status.version <= Constants::APP_VERSION)
            {
                // Only log if there's an actual server message to display
                if (!status.message.empty())
                {
                    StatusUI::AddMessage("INFO: Server message: " + status.message);
                }
                return true; // Status OK, Version OK
            }
            else // Outdated version
            {
                StatusUI::AddMessage("ERROR: Outdated version detected (" + std::to_string(Constants::APP_VERSION) +
                    " < " + std::to_string(status.version) + "). Update required.");
                StatusUI::AddMessage("INFO: Opening download page: " + status.download);
                ShellExecuteA(NULL, "open", status.download.c_str(), NULL, NULL, SW_SHOWNORMAL);
                return false; // Version outdated - fail the check
            }
        }
        else // Application disabled by server
        {
            StatusUI::AddMessage("ERROR: " + std::string(Constants::APP_NAME) + " is currently DISABLED by the server.");
            if (!status.message.empty())
            {
                StatusUI::AddMessage("INFO: Server message: " + status.message); // Show reason if provided
            }
            return false; // Status indicates disabled
        }

    }
    catch (const json::parse_error& e) {
        StatusUI::AddMessage("ERROR: Failed to parse status API response (JSON invalid): " + std::string(e.what()));
        return false;
    }
    catch (const json::type_error& e) {
        StatusUI::AddMessage("ERROR: Invalid data type in status API response (JSON type error): " + std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        StatusUI::AddMessage("ERROR: Exception during JSON processing: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        StatusUI::AddMessage("ERROR: Unknown exception during JSON processing.");
        return false;
    }
}
