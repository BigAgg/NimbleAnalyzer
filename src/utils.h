#pragma once
#include <string>
#include <filesystem>

bool IsNumber(const std::string& input);
bool IsInteger(const std::string& input);
std::pair<std::string, std::string> Splitlines(const std::string& input, const std::string& splitat);
bool StrContains(const std::string& input, const std::string& substring);
void RemoveAllSubstrings(std::string& input, const std::string& toRemove);
std::string GetLastWriteTime(const std::filesystem::path& path);
std::string ExcelSerialToDate(int serial);
