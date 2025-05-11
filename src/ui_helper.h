#pragma once

#include <imgui.h>
#include <string>

namespace ImGui {
	bool InputStringWithHint(std::string& str, const std::string& label, const char* hint = "", ImGuiInputFlags flags = 0);
	bool InputStringWithHint(std::string& str, const char* label, const char* hint = "", ImGuiInputFlags flags = 0);
	bool InputString(std::string& str, const std::string& label, ImGuiInputFlags flags = 0);
	bool InputString(std::string& str, const char* label, ImGuiInputFlags flags = 0);
};
