#ifndef KX_STATUS_H
#define KX_STATUS_H

#include <string>
// Include other dependencies like nlohmann/json if the Status struct is defined here

// Data structure to hold status information from the API
struct Status {
    int status = 0;
    int version = 0;
    std::string message;
    std::string download;
};


class KXStatus {
public:
    // Checks the application status via API.
    // Returns true if status is OK and version is current, false otherwise.
    // Logs progress and errors using StatusUI::AddMessage.
    bool CheckStatus();

private:
    // Parses the JSON response from the API.
    // Returns true if parsing is successful, status is OK, and version is current.
    // Logs details using StatusUI::AddMessage.
    bool ReadStatusJson(const std::string& json_data);
};

#endif // KX_STATUS_H