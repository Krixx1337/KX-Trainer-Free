#ifndef GUI_STYLE_H
#define GUI_STYLE_H

namespace GUIStyle {

    // Applies a custom visual style and color theme to ImGui.
    void ApplyCustomStyle();

    // Loads the primary application font
    bool LoadAppFont(float fontSize = 16.0f);

} // namespace GUIStyle

#endif