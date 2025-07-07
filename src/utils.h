#pragma once
#include <string>
#include <filesystem>
#include "timer.h"

bool IsNumber(const std::string& input);
bool IsInteger(const std::string& input);

// Splits a string into 2 parts at given string
std::pair<std::string, std::string> Splitlines(const std::string& input, const std::string& splitat);
bool StrContains(const std::string& input, const std::string& substring);
bool StrEndswith(const std::string& input, const std::string& ending);
void RemoveAllSubstrings(std::string& input, const std::string& toRemove);
void ReplaceAllSubstrings(std::string& input, const std::string& from, const std::string& to);
bool IsValidUTF8(const std::string& str);
std::string Convert1252ToUTF8(const std::string& input);
std::string ConvertUTF8To1252(const std::string& input);
std::string StrToWstr(const std::string& input);
std::wstring GetWstring(const std::string& input);
std::string GetLastWriteTime(const std::filesystem::path& path);
std::string ExcelSerialToDate(int serial);
