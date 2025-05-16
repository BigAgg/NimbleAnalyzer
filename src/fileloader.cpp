#include "fileloader.h"

#include <xlnt/xlnt.hpp>
#include <filesystem>
#include <fstream>
#include "logging.h"

namespace fs = std::filesystem;

// Function predefinitions
static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename);
static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite = false);
static bool s_CheckFile(const std::string& filename);
static bool s_IsNumber(const std::string& input);
static bool s_IsInteger(const std::string& input);

static bool s_IsNumber(const std::string& input) {
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
		std::cout << e.what() << "\n";
		return false;
	}
	return true;
}

static bool s_IsInteger(const std::string& input) {
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

static bool s_CheckFile(const std::string& filename) {
	try {
		// Converting filename to a path
		fs::path path = filename;
		fs::create_directories("sheets");
		xlnt::workbook wb;
		wb.load(path.wstring());
		wb.save("sheets/to_check.xlsx");
		wb.clear();
		wb.load("sheets/to_check.xlsx");
	}
	catch (std::exception& e) {
		logging::logwarning("FILELOADER::s_CheckFile Error Checking File: %s", e.what());
		return false;
	}
	logging::loginfo("FILELOADER::s_CheckFile File to load is intact: %s", filename);
	return true;
}

static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename) {
	std::vector<std::vector<std::string>> sheetData;
	// Converting filename to a path
	fs::path path = fs::u8path(filename);
	// Checking if the file exists
	if (!fs::exists(path)) {
		logging::logwarning("FILELOADER::s_CheckFile File does not exist: %s", filename.c_str());
		return sheetData;
	}
	if (!s_CheckFile(path.string())) {
		logging::logwarning("FILELOADER::s_LoadExcelSheet Error loading file: %s", filename.c_str());
		return sheetData;
	}
	try {
		std::vector<std::vector<std::string>> testData;
		xlnt::workbook wb;
		fs::path to_load = path.string();
		wb.load(to_load.wstring());
		xlnt::worksheet ws = wb.active_sheet();
		// Loading each row
		for (auto rows : ws.rows(false)) {
			std::vector<std::string> rowdata;
			for (auto cell : rows) {
				std::string value = cell.to_string();
				if (s_IsNumber(value)) {
					std::replace(value.begin(), value.end(), '.', ',');
				}
				rowdata.push_back(value);
			}
			testData.push_back(rowdata);
		}
		sheetData = testData;
	}
	catch(std::exception & e) {
		logging::logerror("FILELOADER::s_LoadExcelSheet Error loading file: %s", e.what());
		sheetData.clear();
	}
	// Load the file with xlnt
	return sheetData;
}

static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite) {
	xlnt::workbook wb;
	fs::path path = fs::u8path(filename);
	if (!overwrite) {
		if (!s_CheckFile(path.string())) {
			logging::logwarning("FILELOADER::s_SaveExcelSheet filechecking failure for: %s", filename.c_str());
			return;
		}
		fs::path to_load = path.string();
		wb.load(path.wstring());
	}
	xlnt::worksheet ws = wb.active_sheet();
	for (int x = 0; x < excelSheet.size(); x++) {
		for (int y = 0; y < excelSheet[x].size(); y++) {
			xlnt::cell_reference cell_ref = xlnt::cell_reference(y + 1, x + 1);
			auto dest_cell = ws.cell(cell_ref);
			// Check if it is a merged cell
			bool skip = false;
			for (const auto& range : ws.merged_ranges()) {
				if (range.contains(cell_ref) && cell_ref != range.top_left()) {
					skip = true;
					break;
				}
			}
			std::string value = excelSheet[x][y];
			//if (value == dest_cell.to_string())
				//skip = true;
			if (value == "")
				skip = true;
			if (skip)
				continue;
			// Asign cell integer value
			if (s_IsInteger(value)) {
				int number = std::stoi(value);
				dest_cell.value(number);
				continue;
			}
			// Asign cell float value
			if (s_IsNumber(value)) {
				replace(value.begin(), value.end(), ',', '.');
				float number = std::stof(value);
				dest_cell.value(number);
				continue;
			}
			// Asign cell value string
			dest_cell.value(value);
		}
	}
	wb.save(filename);
}


void FileInfo::LoadFile(const std::string& filename) {
	m_sheetData = s_LoadExcelSheet(filename);
	// Check if there is any data
	if (m_sheetData.size() <= 0)
		return;
	// Get header index
	int headerIndex = -1;
	int idx = 0;
	for (auto& row : m_sheetData) {
		idx++;
		if (row.size() <= 0)
			continue;
		if (row[0] == "DATA") {
			headerIndex = idx - 1;
		}
	}
	// Check if file is valid
	if (headerIndex < 0) {
		logging::logwarning("FILELOADER::FileInfo::LoadFile Loaded file does not contain 'DATA' in the 'A' Column\n Read the Documentation!");
		return;
	}
	// Processing RowInfo
	for (int x = headerIndex; x < m_sheetData.size(); x++) {
		const auto& row = m_sheetData[x];
		RowInfo rowinfo;
		bool dataSet = false;
		for (int y = 1; y < row.size(); y++) {
			const std::string& header = m_sheetData[headerIndex][y];
			if (header == "")
				continue;
			const std::string& value = m_sheetData[x][y];
			if (value != "")
				dataSet = true;
			rowinfo.AddData(header, value);
		}
		if(dataSet)
			m_rowinfo.push_back(rowinfo);
	}
	m_filename = filename;
	m_isready = true;
}

void FileInfo::SaveFile() {

}

void FileInfo::SaveFileAs(const std::string& filename){
}

std::pair<int, int> FileInfo::GetHeaderIndex(const std::string& header){
	return std::pair<int, int>();
}

void FileInfo::GetHeaderIndex(const std::string& header, int* x, int* y){
}

RowInfo FileInfo::GetRowdata(const int rowIdx){
	return RowInfo();
}

void FileInfo::SetRowData(const int rowIdx){
}

void FileInfo::AddRowData(const RowInfo& rowinfo){
}

bool FileInfo::IsReady() const{
	return m_isready;
}

void RowInfo::AddData(const std::string& header, const std::string& value){
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_rowinfo.end()) {
		m_rowinfo.push_back(std::make_pair(header, value));
		return;
	}
	it->second = value;
}

void RowInfo::UpdateData(const std::string& header, const std::string& newValue){
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_rowinfo.end()) {
		return;
	}
	it->second = newValue;
}

std::string RowInfo::GetData(const std::string& header) const{
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_rowinfo.end()) {
		return "";
	}
	return it->second;
}

std::vector<std::pair<std::string, std::string>> RowInfo::GetData(){
	return m_rowinfo;
}

void RowInfo::SetData(const std::vector<std::pair<std::string, std::string>>& data){
	m_rowinfo = data;
}

void FileSettings::AddHeaderSetting(const std::string& header) {
	auto it = std::find_if(m_headersettings.begin(), m_headersettings.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	if (it == m_headersettings.end()) {
		return;
	}
	it->first = header;
	for (auto& b : it->second) {
		b = false;
	}
}
