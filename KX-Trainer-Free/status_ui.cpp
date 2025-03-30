#include "status_ui.h"
#include "imgui/imgui.h"

namespace StatusUI {

    // Internal state
    static std::vector<std::string> g_statusMessages;
    static std::mutex               g_statusMutex; // Protects g_statusMessages
    static const size_t             MAX_STATUS_MESSAGES = 20; // History limit

    void AddMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        g_statusMessages.push_back(message);
        // Trim old messages if exceeding the limit
        while (g_statusMessages.size() > MAX_STATUS_MESSAGES) {
            g_statusMessages.erase(g_statusMessages.begin());
        }
    }

    void ClearMessages() {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        g_statusMessages.clear();
    }

    std::vector<std::string> GetMessages() {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        return g_statusMessages; // Return a copy
    }

    bool Render(const std::string& error_msg) {
        bool should_exit = false;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowBgAlpha(1.0f); // Solid background

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

        ImGui::Begin("StatusWindow", NULL, window_flags);

        ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
        ImGuiStyle& style = ImGui::GetStyle();

        if (!error_msg.empty()) {
            // --- Error Display ---
            const char* errorTitle = "Initialization Failed!";
            float titleWidth = ImGui::CalcTextSize(errorTitle).x;
            ImGui::SetCursorPosX((contentRegionAvail.x - titleWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", errorTitle);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Error Details:");
            ImGui::PushTextWrapPos(contentRegionAvail.x - style.ItemSpacing.x);
            ImGui::TextWrapped("%s", error_msg.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float buttonWidth = 120.0f;
            float buttonHeight = ImGui::GetFrameHeight();
            float buttonPosX = (contentRegionAvail.x - buttonWidth) * 0.5f;
            float buttonPosY = ImGui::GetWindowHeight() - buttonHeight - style.WindowPadding.y * 2.0f;
            ImGui::SetCursorPos(ImVec2(style.WindowPadding.x + buttonPosX, buttonPosY));

            if (ImGui::Button("Exit Application", ImVec2(buttonWidth, 0))) {
                should_exit = true;
            }

        }
        else {
            // --- Loading Display ---
            const char* loadingTitle = "Initializing KX Trainer...";
            float titleWidth = ImGui::CalcTextSize(loadingTitle).x;
            ImGui::SetCursorPosX((contentRegionAvail.x - titleWidth) * 0.5f);
            ImGui::TextUnformatted(loadingTitle);
            ImGui::Separator();

            // Scrolling child window for status messages
            float childHeight = contentRegionAvail.y - ImGui::GetCursorPosY() - style.WindowPadding.y;
            ImGuiWindowFlags child_flags = ImGuiWindowFlags_HorizontalScrollbar;
            ImGui::BeginChild("StatusLog", ImVec2(0, childHeight), false, child_flags);

            { // Scope for mutex lock
                std::lock_guard<std::mutex> lock(g_statusMutex);
                for (const auto& msg : g_statusMessages) {
                    // Basic coloring based on prefix
                    ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                    if (msg.rfind("ERROR:", 0) == 0)      color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    else if (msg.rfind("WARN:", 0) == 0)  color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
                    else if (msg.rfind("INFO:", 0) == 0)  color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                    ImGui::TextColored(color, "%s", msg.c_str());
                }
            } // Mutex released here

            // Auto-scroll to bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight() * 2) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
        }

        ImGui::End(); // End StatusWindow

        return should_exit;
    }

} // namespace StatusUI