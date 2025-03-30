#ifndef STATUS_UI_H
#define STATUS_UI_H

#include <string>
#include <vector>
#include <mutex> // Required for thread safety implementation detail

namespace StatusUI {

    // Adds a message to the status log (thread-safe).
    void AddMessage(const std::string& message);

    // Renders the initial status/loading/error window using ImGui.
    // Returns true if the user clicks the "Exit" button in the error state.
    bool Render(const std::string& error_msg = "");

    // Clears all messages from the log (thread-safe).
    void ClearMessages();

    // Gets a copy of the current status messages (thread-safe).
    std::vector<std::string> GetMessages();

} // namespace StatusUI

#endif // STATUS_UI_H