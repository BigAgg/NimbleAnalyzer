#include "utils.h"
#include <codecvt>
#include <Windows.h>

std::string GetLastWriteTime(const std::filesystem::path& path) {
	using namespace std::chrono;

	auto ftime = std::filesystem::last_write_time(path);

	// Convert to system_clock time_point
	auto sctp = time_point_cast<system_clock::duration>(
		ftime - decltype(ftime)::clock::now() + system_clock::now());

	std::time_t cftime = system_clock::to_time_t(sctp);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&cftime), "%F %T");  // e.g., 2025-05-23 13:45:00
	return ss.str();
}


bool StrContains(const std::string& input, const std::string& substring) {
	return (input.find(substring) != std::string::npos);
}

bool StrStartswith(const std::string& input, const std::string& start){
	return input.rfind(start, 0) == 0;
}

bool StrEndswith(const std::string& input, const std::string& ending){
	if (ending.size() > input.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), input.rbegin());
}

void RemoveAllSubstrings(std::string& input, const std::string& toRemove) {
	size_t pos;
	while ((pos = input.find(toRemove)) != std::string::npos) {
		input.erase(pos, toRemove.length());
	}
}

void ReplaceAllSubstrings(std::string& input, const std::string& from, const std::string& to){
	if (from.empty()) return;

	std::size_t start_pos = 0;
	while ((start_pos = input.find(from, start_pos)) != std::string::npos) {
		input.replace(start_pos, from.length(), to);
		start_pos += to.length(); // advance past replacement
	}
}

bool IsValidUTF8(const std::string& str){
	int c, i, ix, n, j;
	for (i = 0, ix = str.length(); i < ix; i++) {
		c = (unsigned char)str[i];
		if (c >= 0 && c <= 127) n = 0;
		else if ((c & 0xE0) == 0xC0) n = 1;
		else if ((c & 0xF0) == 0xE0) n = 2;
		else if ((c & 0xF8) == 0xF0) n = 3;
		else return false;
		for (j = 0; j < n && i < ix; j++) {
			if ((++i == ix) || (((unsigned char)str[i] & 0xC0) != 0x80)) return false;
		}
	}
	return true;
}

std::string Convert1252ToUTF8(const std::string& input){
	// Convert Windows-1252 to UTF-16
	int wideLen = MultiByteToWideChar(1252, 0, input.c_str(), -1, NULL, 0);
	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(1252, 0, input.c_str(), -1, &wideStr[0], wideLen);

	// Convert UTF-16 to UTF-8
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	std::string utf8Str(utf8Len, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, NULL, NULL);

	// Remove trailing null terminator
	utf8Str.pop_back();

	return utf8Str;
}

std::string ConvertUTF8To1252(const std::string& input){
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, NULL, 0);
	if (wideLen == 0) return "";

	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &wideStr[0], wideLen);

	int ansiLen = WideCharToMultiByte(1252, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	if (ansiLen == 0) return "";

	std::string ansiStr(ansiLen, 0);
	WideCharToMultiByte(1252, 0, wideStr.c_str(), -1, &ansiStr[0], ansiLen, NULL, NULL);

	ansiStr.pop_back(); // remove null terminator
	return ansiStr;
}

std::string StrToWstr(const std::string& input){
	std::wstring wstr;
	try {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		wstr = converter.from_bytes(input);
	}
	catch (std::range_error& e) {
		size_t length = input.length();
		std::wstring result;
		result.reserve(length);
		for (size_t i = 0; i < length; i++) {
			result.push_back(input[i] & 0xFF);
		}
		wstr = result;
	}
	size_t len = std::wcstombs(nullptr, wstr.c_str(), 0) + 1;
	char* buffer = new char[len];
	std::wcstombs(buffer, wstr.c_str(), len);
	std::string output(buffer);
	delete[] buffer;

	return output;
}

std::wstring GetWstring(const std::string& input){
	try {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(input);
	}
	catch (std::range_error& e) {
		size_t length = input.length();
		std::wstring result;
		result.reserve(length);
		for (size_t i = 0; i < length; i++) {
			result.push_back(input[i] & 0xFF);
		}
		return result;
	}
}

std::pair<std::string, std::string> Splitlines(const std::string& input, const std::string& splitat) {
	size_t pos = input.find(splitat);
	if (pos == std::string::npos) {
		return { input, "" }; // No delimiter found
	}

	std::string left = input.substr(0, pos);
	std::string right = input.substr(pos + splitat.length());

	return { left, right };
}

bool IsNumber(const std::string& input) {
	try{
		if (input.size() == 0) return false;
		std::string str = input;
		std::replace(str.begin(), str.end(), ',', '.');
		// Checking for prefix
		size_t start = 0;
		if (input[0] == '-' || input[0] == '+') {
			if (input.size() == 1) return false;
			start = 1;
		}
		// Checking for any characters
		int dotcount = 0;
		for (size_t i = start; i < input.size(); ++i) {
			if (input[i] > 255 || input[i] < 0)
				return false;
			if (std::isdigit(input[i])) {
				continue;
			}
			else if (input[i] == '.' || input[i] == ',') {
				dotcount++;
				if (dotcount > 1)
					return false;
				continue;
			}
			else {
				return false;
			}
		}
		if (dotcount == 1)
			return true;
	}
	catch (const std::exception& e){
		return false;
	}
	return true;
}

bool IsInteger(const std::string& input) {
	if (input.size() == 0) return false;
	// First make sure there are no invalid characters
	size_t start = 0;
	if (input[0] == '+' || input[0] == '-')
		start = 1;
	for (size_t i = start; i < input.size(); i++) {
		if (input[0] > 255 || input[0] < 0)
			return false;
		if (!std::isdigit(input[i]))
			return false;
	}
	return true;
}

std::string ExcelSerialToDate(int serial) {
	int l = serial + 68569 + 2415019;
	int n = 4 * l / 146097;
	l = l - (146097 * n + 3) / 4;
	int i = 4000 * (l + 1) / 1461001;
	l = l - 1461 * i / 4 + 31;
	int j = 80 * l / 2447;
	int day = l - 2447 * j / 80;
	l = j / 11;
	int month = j + 2 - 12 * l;
	int year = 100 * (n - 49) + i + l;

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << day << "." << std::setw(2) << month << "." << year;
	return oss.str();
}
