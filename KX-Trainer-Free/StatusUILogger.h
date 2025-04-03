#pragma once
#ifndef STATUSUI_LOGGER_H
#define STATUSUI_LOGGER_H

#include "ILogger.h"
#include <string>
#include <vector>

// Concrete implementation of ILogger that uses the existing StatusUI static functions.
class StatusUILogger : public ILogger {
public:
    StatusUILogger() = default;
    virtual ~StatusUILogger() override = default;

    // Log a simple string message (prefix determines level implicitly for StatusUI)
    virtual void Log(const std::string& message) override;

    // Log a message with a specific level (adds prefix for StatusUI)
    virtual void Log(LogLevel level, const std::string& message) override;

    // Retrieve stored messages from StatusUI
    virtual std::vector<std::string> GetMessages() const override;

    // Clear stored messages in StatusUI
    virtual void ClearMessages() override;

private:
    // Helper to convert LogLevel to string prefix
    std::string LogLevelToString(LogLevel level) const;
};

#endif // STATUSUI_LOGGER_H