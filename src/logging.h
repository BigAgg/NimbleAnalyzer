#pragma once

#include <memory>
#include <string>
#include <vector>

namespace strings {
bool ends_with(const std::string &value, const std::string &ending);
std::string GetTimestamp();

template <typename T>
auto format_arg(T&& value) {
  if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
    return value.c_str();
  else
    return std::forward<T>(value);
}

template<typename... Args>
std::string formatString(const char* format, Args&&... args) {
  int size = std::snprintf(nullptr, 0, format, format_arg(std::forward<Args>(args))...) + 1;	// Get Required Size
  if (size <= 0) return "";

  std::unique_ptr<char[]> buffer(new char[size]);
  std::snprintf(buffer.get(), size, format, format_arg(std::forward<Args>(args))...);

  return std::string(buffer.get(), buffer.get() + size - 1);
}
// Overload function for strings
template<typename... Args>
std::string formatString(const std::string& format, Args&&... args) {
  return formatString(format.c_str(), std::forward<Args>(args)...);
}
} // namespace strings

namespace logging {
void log(const std::string &type, const std::string &msg);
template <typename... Args>
void loginfo(const std::string &msg, Args &&...args) {
  log("INFO", strings::formatString(msg, args...));
}

template <typename... Args>
void logwarning(const std::string &msg, Args &&...args) {
  log("WARNING", strings::formatString(msg, args...));
}

template <typename... Args>
void logerror(const std::string &msg, Args &&...args) {
  log("ERROR", strings::formatString(msg, args...));
}

void startlogging(const std::string &path, const std::string &filename);
void stoplogging();
void backuplog(const std::string &path, bool crash = false);
void deletelog(const std::string &path);
std::string GetLastError();
std::string GetLastWarning();
std::vector<std::string> GetErrors();
std::vector<std::string> GetWarnings();
} // namespace logging
