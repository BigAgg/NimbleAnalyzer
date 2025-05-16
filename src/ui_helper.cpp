#include "ui_helper.h"

#include <imgui.h>
#include <string>

namespace ImGui {
bool InputStringWithHint(std::string& str, const std::string& label, const char* hint, ImGuiInputTextFlags flags) {
	char value_buff[256];
	char label_buff[256];
	strncpy_s(value_buff, str.c_str(), 256);
	strncpy_s(label_buff, label.c_str(), 256);
	if (InputTextWithHint(label_buff, hint, value_buff, 256, flags)) {
		str = std::string(value_buff);
		return true;
	}
	return false;
}

bool InputStringWithHint(std::string& str, const char* label, const char* hint, ImGuiInputTextFlags flags) {
	char value_buff[256];
	strncpy_s(value_buff, str.c_str(), 256);
	if (InputTextWithHint(label, hint, value_buff, 256, flags)) {
		str = std::string(value_buff);
		return true;
	}
	return false;
}

bool InputString(std::string& str, const std::string& label, ImGuiInputTextFlags flags) {
	char value_buff[256];
	char label_buff[256];
	strncpy_s(value_buff, str.c_str(), 256);
	strncpy_s(label_buff, label.c_str(), 256);
	if (InputText(label_buff, value_buff, 256, flags)) {
		str = std::string(value_buff);
		return true;
	}
	return false;
}

bool InputString(std::string& str, const char* label, ImGuiInputTextFlags flags) {
	char value_buff[256];
	strncpy_s(value_buff, str.c_str(), 256);
	if (InputText(label, value_buff, 256, flags)) {
		str = std::string(value_buff);
		return true;
	}
	return false;
}
};
