#pragma once
#ifndef ILOGGER_H
#define ILOGGER_H

#include <string>
#include <vector> // Include vector for GetMessages potentially

// Define different log levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical // Added for more severe errors
};

class ILogger {
public:
    virtual ~ILogger() = default; // Virtual destructor is important for interfaces

    // Log a simple string message (implicitly Info level or requires prefix)
    virtual void Log(const std::string& message) = 0;

    // Log a message with a specific level (preferred method)
    virtual void Log(LogLevel level, const std::string& message) = 0;

    // Optional: Method to retrieve stored messages if the logger keeps history
    // This might be useful for displaying logs in the UI without coupling UI to StatusUI
    virtual std::vector<std::string> GetMessages() const = 0;

    // Optional: Method to clear stored messages
    virtual void ClearMessages() = 0;
};

#endif // ILOGGER_H