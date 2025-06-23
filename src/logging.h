#pragma once

#include <memory>
#include <string>
#include <vector>

// Maybe put this into utils????
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

// Formats a given string with given arguments
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
// Simple log function that should not be used outside unless you want to use your own types
void log(const std::string &type, const std::string &msg);
// Logs an info to the console std::cout
template <typename... Args>
void loginfo(const std::string &msg, Args &&...args) {
  log("INFO", strings::formatString(msg, args...));
}
// Logs a warning to the console std::cerr
template <typename... Args>
void logwarning(const std::string &msg, Args &&...args) {
  log("WARNING", strings::formatString(msg, args...));
}
// Logs an error to the console std::cerr
template <typename... Args>
void logerror(const std::string &msg, Args &&...args) {
  log("ERROR", strings::formatString(msg, args...));
}

// Start logging to a file
void startlogging(const std::string &path, const std::string &filename);
// stop logging
void stoplogging();
// backup a logfile and back it up as a crash.log if it was a crash
void backuplog(const std::string &path, bool crash = false);
// delete a logfile
void deletelog(const std::string &path);
// Return the last error that was logged
std::string GetLastError();
// Returnt he last warning that was logged
std::string GetLastWarning();
// Get the whole errorlist
std::vector<std::string> GetErrors();
// Get the whole warnings list
std::vector<std::string> GetWarnings();
} // namespace logging
