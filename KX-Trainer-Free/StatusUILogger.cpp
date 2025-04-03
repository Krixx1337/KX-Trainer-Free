#include "StatusUILogger.h"
#include "status_ui.h"

// For StatusUI, the level is often inferred from prefixes like "INFO:", "WARN:", "ERROR:".
// This simple Log method will just pass the message through as is.
void StatusUILogger::Log(const std::string& message) {
    StatusUI::AddMessage(message);
}

// We prepend a standard prefix so StatusUI can potentially color it.
void StatusUILogger::Log(LogLevel level, const std::string& message) {
    StatusUI::AddMessage(LogLevelToString(level) + message);
}

std::vector<std::string> StatusUILogger::GetMessages() const {
    return StatusUI::GetMessages();
}

void StatusUILogger::ClearMessages() {
    StatusUI::ClearMessages();
}

std::string StatusUILogger::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:    return "DEBUG: ";
        case LogLevel::Info:     return "INFO: ";
        case LogLevel::Warning:  return "WARN: ";
        case LogLevel::Error:    return "ERROR: ";
        case LogLevel::Critical: return "CRITICAL: ";
        default:                 return "LOG: "; // Default prefix
    }
}