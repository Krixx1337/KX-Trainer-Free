#pragma once

#include <string>

class KXStatus
{
public:
    bool CheckStatus();

private:
    bool ReadStatusJson(std::string readBuffer);

    struct Status
    {
        int status{ 0 };
        int version{ 0 };
        std::string message;
        std::string download;
    };
};
