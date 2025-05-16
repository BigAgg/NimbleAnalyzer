#pragma once

#include <imgui.h>
#include <string>

namespace ImGui {
	bool InputStringWithHint(std::string& str, const std::string& label, const char* hint = "", ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
	bool InputStringWithHint(std::string& str, const char* label, const char* hint = "", ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
	bool InputString(std::string& str, const std::string& label, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
	bool InputString(std::string& str, const char* label, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
};
