#include "utils.h"

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

void RemoveAllSubstrings(std::string& input, const std::string& toRemove) {
	size_t pos;
	while ((pos = input.find(toRemove)) != std::string::npos) {
		input.erase(pos, toRemove.length());
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
