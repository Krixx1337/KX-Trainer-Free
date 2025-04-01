#pragma once

// Returns a human-readable string representation of a Windows virtual key code.
// Returns "None" for VK code 0.
// Returns "VK 0xXX" for unknown codes.
const char* GetKeyName(int vk_code);