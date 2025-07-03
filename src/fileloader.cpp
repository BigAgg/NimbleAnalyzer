/*
MIT License

Copyright (c) 2025 Adrian Jahraus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "fileloader.h"

#include <xlnt/xlnt.hpp>
#include <filesystem>
#include <fstream>
#include <format>
#include "logging.h"
#include "utils.h"
#include "utf8.h"
#include <unordered_set>
#include <codecvt>

namespace fs = std::filesystem;

// Function predefinitions
// Checks if a file is intact or not
static bool s_CheckFile(const std::string& filename);
std::vector<std::vector<std::string>> s_LoadCSVSheet(const std::string& filename);
static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename);
static void s_SaveCSVSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite = false, const std::string& sourcefile = "");
static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite = false, const std::string& sourcefile = "");

static bool s_CheckFile(const std::string& filename) {
	try {
		// Converting filename to a path
		fs::path path = filename;
		fs::create_directories("sheets");
		xlnt::workbook wb;
		// try to load / save and again load the file
		wb.load(path.wstring());
		wb.save("sheets/to_check.xlsx");
		wb.clear();
		wb.load("sheets/to_check.xlsx");
	}
	catch (std::exception& e) {
		logging::logwarning("FILELOADER::s_CheckFile Error Checking File: %s", e.what());
		return false;
	}
	return true;
}

std::vector<std::vector<std::string>> s_LoadCSVSheet(const std::string& filename) {
	std::vector<std::vector<std::string>> sheetData;
	fs::path path = fs::u8path(filename);

	if (!fs::exists(path)) {
		logging::logwarning("FILELOADER::s_LoadCSVSheet File does not exist: %s", filename.c_str());
		return sheetData;
	}

	try {
		// Read entire file into a string (binary, no interpretation yet)
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			logging::logerror("FILELOADER::s_LoadCSVSheet File could not be opened: %s", filename.c_str());
			return sheetData;
		}

		std::string rawContent((std::istreambuf_iterator<char>(file)), {});
		std::string fileContent = Convert1252ToUTF8(rawContent);

		// Remove UTF-8 BOM if present
		if (fileContent.size() >= 3 &&
			(unsigned char)fileContent[0] == 0xEF &&
			(unsigned char)fileContent[1] == 0xBB &&
			(unsigned char)fileContent[2] == 0xBF) {
			fileContent = fileContent.substr(3);
		}

		std::istringstream contentStream(fileContent);
		std::string line;
		std::string separator = ";";
		separator.erase(0, separator.find_first_not_of(" \t\r\n"));
		separator.erase(separator.find_last_not_of(" \t\r\n") + 1);

		int x = 0;
		while (std::getline(contentStream, line)) {
			// Clean up line
			RemoveAllSubstrings(line, "\"");
			RemoveAllSubstrings(line, "\n");
			RemoveAllSubstrings(line, "\t");
			RemoveAllSubstrings(line, "\r");
			ReplaceAllSubstrings(line, "\\", "/");

			// Check for sep= directive
			if (x == 0 && line.starts_with("sep=")) {
				separator = Splitlines(line, "=").second;
				separator.erase(0, separator.find_first_not_of(" \t\r\n"));
				separator.erase(separator.find_last_not_of(" \t\r\n") + 1);
				x++;
				continue;
			}

			std::vector<std::string> row;
			std::pair<std::string, std::string> values = Splitlines(line, separator);

			while (StrContains(values.second, separator)) {
				row.push_back(values.first);
				values = Splitlines(values.second, separator);
			}

			// Push remaining values
			row.push_back(values.first);
			row.push_back(values.second);

			sheetData.push_back(row);
			x++;
		}
	}
	catch (const std::exception& e) {
		logging::logerror("FILELOADER::s_LoadCSVSheet Could not load file: %s\nERROR: %s", filename.c_str(), e.what());
		return {};
	}

	return sheetData;
}

static std::vector<std::vector<std::string>> s_LoadExcelSheet(const std::string& filename) {
	std::vector<std::vector<std::string>> sheetData;
	// Converting filename to a path
	fs::path path = fs::u8path(filename);
	const std::string extension = path.filename().extension().string();
	// Check if the file is a csv and call its load function instead
	if (extension == ".csv" || extension == ".CSV") {
		return s_LoadCSVSheet(filename);
	}
	// Checking if the file exists
	if (!fs::exists(path)) {
		logging::logwarning("FILELOADER::s_LoadExcelSheet File does not exist: %s", filename.c_str());
		return sheetData;
	}
	// Check if the file is loadable
	if (!s_CheckFile(path.string())) {
		logging::logwarning("FILELOADER::s_LoadExcelSheet Error loading file: %s", filename.c_str());
		return sheetData;
	}
	try {
		std::vector<std::vector<std::string>> testData;	// Used to not break sheetData if the try fails
		xlnt::workbook wb;
		fs::path to_load = path.string();
		wb.load(to_load.wstring());		// Loading excel sheet
		xlnt::worksheet ws = wb.active_sheet();	// Getting the active sheet
		// Loading each row
		for (auto rows : ws.rows(false)) {
			std::vector<std::string> rowdata;
			for (auto cell : rows) {
				std::string value = cell.to_string();
				// Check if the value is a float and convert it to be 3 digits precision and convert '.' to ',' (german convertings)
				if (!IsInteger(value) && cell.has_value() && cell.data_type() == xlnt::cell_type::number) {
					float number = cell.value<float>();
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(3) << number;
					value = oss.str();
					std::replace(value.begin(), value.end(), '.', ',');
				}
				rowdata.push_back(value);
			}
			testData.push_back(rowdata);
		}
		sheetData = testData;		// Now everything is loaded and it didnt crash so asign the testData
	}
	catch(std::exception & e) {
		logging::logerror("FILELOADER::s_LoadExcelSheet Error loading file: %s", e.what());
		sheetData.clear();
	}
	return sheetData;
}

static void s_SaveCSVSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite, const std::string& sourcefile) {
	// generate the path
	fs::path path = fs::u8path(filename);
	// Open the file
	std::ofstream file(path.wstring(), std::ios::binary);
	if (!file) {
		logging::logwarning("FILELOADER::s_SaveCSVSheet Could not open file: %s", filename.c_str());
		return;
	}
	// Set the separator to be ';'
	file << ConvertUTF8To1252("sep=;\r\n");
	// Iterate each row and generate the csv style behavior
	std::string comma = ",";
	comma.erase(0, comma.find_first_not_of(" \t\r\n"));
	comma.erase(comma.find_last_not_of(" \t\r\n") + 1);
	for (auto&& row : excelSheet) {
		for (int x = 0; x < row.size(); x++) {
			std::string val = ConvertUTF8To1252(row[x]);	// Get the data
			ReplaceAllSubstrings(val, "\n", " ");
			if (x == 0) {
				if (val == "")
					continue;
				// Check if its a number or int else write the value inside '"'
				if (IsInteger(val) || IsNumber(val) && StrContains(val, comma)) {
					file << val;
				}
				else {
					file << '"' << val << '"';
				}
			}
			else {
				file << ';';
				if (val == "")
					continue;
				if (IsInteger(val) || IsNumber(val) && StrContains(val, comma)) {
					file << val;
				}
				else {
					file << '"' << val << '"';
				}
			}
		}
		file << "\r\n";	// This row is done, start a new one
	}
}

static void s_SaveExcelSheet(const std::string& filename, const std::vector<std::vector<std::string>>& excelSheet, const bool overwrite, const std::string& sourcefile) {
	// Generate the path
	fs::path path = fs::u8path(filename);
	// Get what extension the file has and load csv if extension matches it
	const std::string extension = path.filename().extension().string();
	if (extension == ".csv" || extension == ".CSV") {
		s_SaveCSVSheet(filename, excelSheet, overwrite, sourcefile);
		return;
	}
	// Generate path for source file
	fs::path sourcepath = fs::u8path(sourcefile);
	xlnt::workbook wb;
	// Load the destfile if you dont want to overwrite the selected file
	if (!overwrite) {
		if (!s_CheckFile(path.string())) {
			logging::logwarning("FILELOADER::s_SaveExcelSheet filechecking failure for: %s", filename.c_str());
			return;
		}
		wb.load(path.wstring());
	}
	// Check if a sourcefile should be loaded and load it
	if (sourcefile != "") {
		if (!s_CheckFile(path.string())) {
			logging::logwarning("FILELOADER::s_SaveExcelSheet filecheking failure for sourcefile: %s", sourcefile.c_str());
		}
		else {
			wb.load(sourcepath.wstring());
		}
	}
	xlnt::worksheet ws = wb.active_sheet();
	// clearing everything that comes after the sheet
	// Determine actual size
	const std::size_t max_row = excelSheet.size();
	std::size_t max_col = 0;
	for (const auto& row : excelSheet) {
		if (row.size() > max_col)
			max_col = row.size();
	}

	// Get the worksheet's current used range
	auto used_range = ws.calculate_dimension();
	const std::size_t used_max_row = used_range.bottom_right().row();
	const std::size_t used_max_col = used_range.bottom_right().column().index;

	// Clear rows beyond max_row
	for (std::size_t row = max_row + 1; row <= used_max_row; ++row) {
		for (std::size_t col = 1; col <= used_max_col; ++col) {
			ws.cell(xlnt::cell_reference(col, row)).clear_value();
		}
	}

	// Clear columns beyond max_col in used rows
	for (std::size_t row = 1; row <= max_row; ++row) {
		for (std::size_t col = max_col + 1; col <= used_max_col; ++col) {
			ws.cell(xlnt::cell_reference(col, row)).clear_value();
		}
	}

	// Iterate and write to the excel sheet's cells
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
			if (skip)
				continue;
			// Retrieve data from the excelSheet that should be written into the cell
			std::string value = excelSheet[x][y];
			// Asign cell integer value
			if (IsInteger(value)) {
				int number = std::stoi(value);
				dest_cell.value(number);
				continue;
			}
			// Asign cell float value
			std::string comma = ",";
			comma.erase(0, comma.find_first_not_of(" \t\r\n"));
			comma.erase(comma.find_last_not_of(" \t\r\n") + 1);

			if (IsNumber(value) && StrContains(value, comma)) {
				std::replace(value.begin(), value.end(), ',', '.');
				float number = std::stof(value);
				dest_cell.value(number);
				dest_cell.number_format(xlnt::number_format::number_format("0.000")); // 3 decimal digits
				continue;
			}
			// Asign cell value string
			if (!IsValidUTF8(value)) {
				std::string cleaned;
				utf8::replace_invalid(value.begin(), value.end(), std::back_inserter(cleaned));
				dest_cell.value(cleaned);
			}
			else {
				dest_cell.value(value);
				if(value != "")
					dest_cell.number_format(xlnt::number_format::text());
			}
		}
	}
	// Save the file
	try {
		wb.save("sheets/to_save.xlsx");
		if (s_CheckFile("sheets/to_save.xlsx"))
			wb.save(filename);
		else
			logging::logerror("FILELOADER::s_SaveExcelSheet File got corrupetd: %s", filename.c_str());
	}
	catch (std::exception& e) {
		logging::logerror("FILELOADER::s_SaveExcelSheet File could not be saved: %s", e.what());
	}
}

void FileInfo::Unload() {
	if (!IsReady())
		return;
	// Unload all data to clear memory
	for (auto& rinfo : m_rowinfo) {
		rinfo.Unload();
	}
	m_rowinfo.clear();
	Settings->Unload();
	m_sheetData.clear();
	m_headerinfo.clear();
	m_filename = "";
	m_isready = false;
	m_headeridx = -1;
}

void FileInfo::LoadSettings(const std::string& path){
	// Fix the string path to a u8path
	fs::path fixedpath = fs::u8path(path);
	// Load and check the file
	std::ifstream file(fixedpath.wstring(), std::ios::binary);
	if (!file) {
		logging::logwarning("FILELOADER::FileInfo::LoadSettings Could not load File Settings: %s", path.c_str());
		return;
	}
	// Read the file line by line
	std::string line;
	while (std::getline(file, line)) {
		RemoveAllSubstrings(line, "\n");	// Needed as at the end of every line there is a \n which will be converted to the value if not removed
		std::pair<std::string, std::string> lineValues = Splitlines(line, " = ");	// Split the line into header and value 
		// For easy access
		const std::string header = lineValues.first;
		const std::string value = lineValues.second;
		// Now check which data should be asigned and asign it
		if (header == "m_filename") {
			m_filename = value;
		}
		else if (header == "m_mergefile" && value != "") {
			FileInfo mergefile;
			mergefile.LoadFile(value);
			Settings->SetMergeFile(mergefile);
		}
		else if (header == "m_mergefolderfile" && value != "") {
			Settings->SetMergeFolderTemplate(value);
		}
		else if (header == "m_dontimportifexistsheader") {
			Settings->SetDontImportIf(value);
		}
		else if (header == "m_mergefolder" && value != "") {
			Settings->SetMergeFolder(value);
		}
		else if (header == "m_mergeheadersfolder") {
			const int amount = std::stoi(value);
			for (int x = 0; x < amount; x++) {
				std::string tmpLine;
				std::getline(file, tmpLine);
				RemoveAllSubstrings(tmpLine, "\n");
				std::pair<std::string, std::string> headerPair = Splitlines(tmpLine, " := ");
				Settings->AddFolderHeaderToMerge(headerPair.first, headerPair.second);
			}
		}
		else if (header == "m_mergefolderif") {
			std::pair<std::string, std::string> mergefolderif = Splitlines(value, " := ");
			Settings->SetMergeFolderHeaderIf(mergefolderif.first, mergefolderif.second);
		}
		else if (header == "m_mergeheaders") {
			const int amount = std::stoi(value);
			for (int x = 0; x < amount; x++) {
				std::string tmpLine;
				std::getline(file, tmpLine);
				RemoveAllSubstrings(tmpLine, "\n");
				std::pair<std::string, std::string> mergeheader = Splitlines(tmpLine, " := ");
				Settings->AddHeaderToMerge(mergeheader.first, mergeheader.second);
			}
		}
		else if (header == "m_mergeif") {
			std::pair<std::string, std::string> mergeif = Splitlines(value, " := ");
			Settings->SetMergeHeaderIf(mergeif.first, mergeif.second);
		}
	}
}

void FileInfo::SaveSettings(const std::string& path){
	if (!IsReady())
		return;
	// Creating the path
	fs::path filePath = fs::u8path(m_filename);
	const std::string filename = path + "/" + filePath.filename().string() + ".ini";
	// Opening the file
	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		logging::logwarning("FILELOADER::FileInfo::SaveSettings Could not save File settings, file cannot be opened!");
		return;
	}
	// Saving all Settings to the file
	file << "m_filename = " << m_filename << '\n';
	file << "m_mergefile = " << Settings->GetMergeFile().GetFilename() << '\n';
	file << "m_mergefolderfile = " << Settings->GetMergeFolderTemplate().GetFilename() << '\n';
	file << "m_dontimportifexistsheader = " << Settings->GetDontImportIf() << '\n';
	file << "m_mergefolder = " << Settings->GetMergeFolder() << '\n';
	auto&& mergefHeaders = Settings->GetMergeFolderHeaders();
	file << "m_mergeheadersfolder = " << mergefHeaders.size() << '\n';
	for (auto&& pair : mergefHeaders) {
		file << pair.first << " := " << pair.second << '\n';
	}
	auto&& mergefolderif = Settings->GetMergeFolderIf();
	file << "m_mergefolderif = " << mergefolderif.first << " := " << mergefolderif.second << '\n';
	auto&& mergeheaders = Settings->GetMergeHeaders();
	file << "m_mergeheaders = " << mergeheaders.size() << '\n';
	for (auto&& pair : mergeheaders) {
		file << pair.first << " := " << pair.second << '\n';
	}
	auto&& mergeif = Settings->GetMergeIf();
	file << "m_mergeif = " << mergeif.first << " := " << mergeif.second << '\n';
}

void SplitWorksheets(const std::string& filename){
	if (!StrEndswith(filename, ".xlsx"))
		return;
	try {
		fs::path path = fs::u8path(filename);
		xlnt::workbook wb;
		wb.load(path.wstring());

		int sheet_index = 0;
		for (const auto& sheet_name : wb.sheet_titles()) {
			xlnt::worksheet ws = wb.sheet_by_title(sheet_name);

			// Create a new workbook and add the current sheet to it
			xlnt::workbook new_wb;
			xlnt::worksheet new_ws = new_wb.active_sheet();
			new_ws.title(sheet_name);

			// Copy contents cell by cell
			for (auto row : ws.rows(false)) {
				for (auto cell : row) {
					new_ws.cell(cell.reference()).value(cell.to_string());
				}
			}
			// Save to file with dynamic name
			std::string output_filename = "sheets/sheet_" + std::to_string(sheet_index) + "_" + sheet_name + ".xlsx";
			new_wb.save(output_filename);
			logging::loginfo("FILELOADER::SplitWorksheets Saved splitfile: \n%s", output_filename.c_str());

			++sheet_index;
		}
	}
	catch(std::exception & e) {
		logging::logerror("%s", e.what());
	}
}

void EditWorksheet(const std::string& filename, int DATA_row, bool deleteEmptyRows){
	xlnt::workbook wb;
	wb.load(filename);
	xlnt::worksheet ws = wb.active_sheet();
	if (DATA_row > 0) {
		ws.insert_columns(1, 1);
		const std::string idx = "A" + std::to_string(DATA_row);
		ws.cell(idx).value("DATA");
		if (deleteEmptyRows) {
			ws.delete_rows(DATA_row + 1, 1);
			// Delete all empty rows (from bottom to top to keep indices stable)
			if (DATA_row <= 0)
				DATA_row = 1;
		}
	}
	wb.save(filename);
}

void FileInfo::LoadFile(const std::string& filename) {
	if (IsReady())
		Unload();
	// Clear everything before loading save is save
	m_sheetData.clear();
	m_rowinfo.clear();
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
	// Processing headerinfo
	m_headeridx = headerIndex;
	for (int y = 1; y < m_sheetData[headerIndex].size(); y++) {
		std::pair<int, int> index = std::make_pair(headerIndex, y);
		std::string header = m_sheetData[headerIndex][y];
		header += " ##" + std::to_string(m_headerinfo.size());
		m_headerinfo.push_back(std::make_pair(header, index));
	}
	// Processing RowInfo
	for (int x = headerIndex + 1; x < m_sheetData.size(); x++) {
		const auto& row = m_sheetData[x];
		RowInfo rowinfo;
		bool dataSet = false;
		for (int y = 1; y < row.size(); y++) {
			const std::string& header = m_headerinfo[y-1].first;
			if (header == "")
				continue;
			std::string& value = m_sheetData[x][y];
			if (value != "")
				dataSet = true;
			// Only way for me by now to check if the loaded shit is a date or not
			if (StrContains(header, "Date")
				|| StrContains(header, "Datum")
				|| StrContains(header, "datum")
				|| StrContains(header, "date")) {
				if (IsNumber(value))
					value = ExcelSerialToDate(std::stoi(value));
			}
			rowinfo.AddData(header, value);
		}
		if(dataSet)
			m_rowinfo.push_back(rowinfo);
		else {
			break;
		}
	}
	// Check if there was no data at all and add one filler data to not crash when saving lol
	if (m_rowinfo.size() == 0) {
		RowInfo rinfo;
		for (auto&& header : m_headerinfo) {
			rinfo.AddData(header.first, "empty_file " + header.first);
		}
		m_rowinfo.push_back(rinfo);
	}
	Settings = new FileSettings();
	Settings->SetParentFile(this);
	m_filename = filename;
	m_isready = true;
}

void FileInfo::SaveFile(const std::string& filename) {
	CreateSheetData();	// convert RowInfo to the m_sheetData
	if (filename == "")
		s_SaveExcelSheet(m_filename, m_sheetData, false);
	else
		s_SaveExcelSheet(filename, m_sheetData, true);
}

void FileInfo::SaveFileAs(const std::string& sourcefile, const std::string& destfile) {
	if (!IsReady()) {
		logging::logwarning("FILELOADER::FileInfo::SaveFileAs File was never loaded correctly. No Data to save");
		return;
	}

	CreateSheetData();	// convert RowInfo to the m_sheetData

	s_SaveExcelSheet(destfile, m_sheetData, false, sourcefile);
}

void FileInfo::CreateSheetData() {
	if (m_rowinfo.size() <= 0) {
		return;	// no data to create
	}
	// Check if the sheetData is not even loaded
	if (m_sheetData.size() <= 0) {
		// Generate header row
		std::vector<std::string> headerRow;
		headerRow.push_back("DATA");
		RowInfo tmp = m_rowinfo[0];
		for (auto& header : GetHeaderNames()) {
			const std::string fixedHeader = Splitlines(header, " ##").first;
			headerRow.push_back(fixedHeader);
		}
		m_sheetData.push_back(headerRow);
		m_headeridx = 0;
		for (auto& hinfo : m_headerinfo) {
			hinfo.second.first = 0;
		}
	}
	m_sheetData.resize(m_headeridx+1);	// Delete all data after the headers to overwrite them properly
	size_t header_size = m_sheetData[m_headeridx].size();
	// Generate all rows for m_sheetData
	for (int x = 0; x < m_rowinfo.size(); x++) {
		const RowInfo& ri = m_rowinfo[x];
		auto&& rdata = ri.GetData();
		// generate row
		std::vector<std::string> rowinfo(m_headerinfo.size() + 1);
		for (auto& pair : rdata) {
			int header_x = -1, header_y = -1;
			GetHeaderIndex(pair.first, &header_x, &header_y);
			if (header_x != m_headeridx)
				continue;
			if (header_y <= 0)
				continue;
			rowinfo[header_y] = pair.second;
		}
		m_sheetData.push_back(rowinfo);
	}
}

std::string FileInfo::GetFilename() const {
	return m_filename;
}

std::pair<int, int> FileInfo::GetHeaderIndex(const std::string& header) {
	// Maybe convert this to std::find_if? but it works for now
	for (auto&& pair : m_headerinfo) {
		if (pair.first == header) {
			return pair.second;
		}
	}
	return std::pair<int, int>(-1, -1);
}

void FileInfo::GetHeaderIndex(const std::string& header, int* x, int* y) {
	for (auto&& pair : m_headerinfo) {
		if (pair.first == header) {
			*x = pair.second.first;
			*y = pair.second.second;
			return;
		}
	}
}

std::vector<std::string> FileInfo::GetHeaderNames() const {
	std::vector<std::string> headernames;
	for (auto& pair : m_headerinfo) {
		headernames.push_back(pair.first);
	}
	return headernames;
}

std::vector<std::pair<std::string, std::pair<int, int>>> FileInfo::GetHeaderInfo(){
	return m_headerinfo;
}

void FileInfo::SetHeaderInfo(std::vector<std::pair<std::string, std::pair<int, int>>> headerinfo){
	m_headerinfo = headerinfo;
}

RowInfo FileInfo::GetRowdata(const int rowIdx){
	if(rowIdx >= m_rowinfo.size())
		return RowInfo();
	return m_rowinfo[rowIdx];
}

std::vector<RowInfo> FileInfo::GetData() {
	return m_rowinfo;
}

void RowInfo::Unload() {
	m_rowinfo.clear();
	m_changed = false;
}

void FileInfo::SetRowData(const RowInfo& rowinfo, const int rowIdx){
	if (rowIdx >= m_rowinfo.size())
		return;
	m_rowinfo[rowIdx] = rowinfo;
}

void FileInfo::AddRowData(const RowInfo& rowinfo){
	m_rowinfo.push_back(rowinfo);
}

void FileInfo::RemoveData(const int rowIdx){
	if (rowIdx >= m_rowinfo.size())
		return;
	m_rowinfo.erase(m_rowinfo.begin() + rowIdx);
}

void FileInfo::ClearData(){
	m_rowinfo.clear();
}

bool FileInfo::IsReady() const{
	return m_isready;
}

void RowInfo::AddData(const std::string& header, const std::string& value){
	auto it = std::find_if(m_rowinfo.begin(), m_rowinfo.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	// Only add it if the header does not exist else edit the value
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
		return;	// Header is not present, so dont update
	}
	it->second = newValue;
	m_changed = true;
}

std::string RowInfo::GetData(const std::string& header) const{
	for (auto& p : m_rowinfo) {
		if (p.first == header)
			return p.second;
	}
	return "";	// header does not exit return empty
}

std::vector<std::pair<std::string, std::string>> RowInfo::GetData() const{
	return m_rowinfo;
}

void RowInfo::SetData(const std::vector<std::pair<std::string, std::string>>& data){
	m_rowinfo = data;
}

bool RowInfo::Changed() {
	bool tmp = m_changed;
	m_changed = false;
	return tmp;
}

void RowInfo::ResetChanged() {
	m_changed = false;
}

void FileSettings::Unload() {
	m_parentFile = nullptr;
	if(m_mergefileSet)
		m_mergefile.Unload();
	m_mergefileSet = false;
	m_mergeheaders.clear();
	m_mergeif = {};
	if (m_mergefolderfile.IsReady())
		m_mergefolderfile.Unload();
	m_mergefolder = "";
	m_mergefolderSet = false;
	m_mergefolderpaths.clear();
}

void FileSettings::SetMergeFile(const FileInfo otherFile) {
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFile m_parentFile is currently not set!");
		return;
	}
	if (!otherFile.IsReady()) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFile Given File is not a valid file.\n%s", otherFile.GetFilename());
		return;
	}
	m_mergefile = otherFile;
	m_mergefileSet = true;
}

FileInfo FileSettings::GetMergeFile() const {
	return m_mergefile;
}

void FileSettings::SetParentFile(FileInfo* parentFile) {
	if (!parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetParentFile Given File is a nullptr!");
		return;
	}
	m_parentFile = parentFile;
}

bool FileSettings::IsMergeFileSet() const{
	return m_mergefileSet;
}

void FileSettings::AddHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader) {
	if (destHeader != "" && !IsMergeFileSet()) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_mergefile is not set yet!");
		return;
	}
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_parentFile is not set yet!");
		return;
	}
	// Check if it already exists and just update
	for (auto& pair : m_mergeheaders) {
		if (pair.first == sourceHeader) {
			pair.second = destHeader;
			return;
		}
	}
	// push back new
	m_mergeheaders.push_back(std::make_pair(sourceHeader, destHeader));
}

void FileSettings::SetMergeHeaderIf(const std::string& sourceHeader, const std::string& destHeader) {
	m_mergeif = std::make_pair(sourceHeader, destHeader);
}

void FileSettings::RemoveHeaderToMerge(const std::string& header) {
	auto it = std::find_if(m_mergeheaders.begin(), m_mergeheaders.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	// if the header doesnt exist, nothing to remove and return
	if (it == m_mergeheaders.end()) {
		return;
	}
	m_mergeheaders.erase(it);
}

std::pair<std::string, std::string> FileSettings::GetMergeIf() const{
	return m_mergeif;
}


std::vector<std::pair<std::string, std::string>> FileSettings::GetMergeHeaders() const{
	return m_mergeheaders;
}

void FileSettings::MergeFiles() {
	std::unordered_set<std::string> dontimportvalues;	// Set to check for the condition header to NOT import
	if (m_dontimportifexistsheader != "" && m_dontimportifexistsheader != "NONE") {
		for (auto& finfo : m_parentFile->GetData()) {
			dontimportvalues.insert(finfo.GetData(m_dontimportifexistsheader));
		}
	}
	int cellsImported = 0;
	if (IsMergeFolderSet() && IsMergeFolderTemplate()) {
		logging::loginfo("FILELOADER::FileSettings::MergeFiles merging all files from folder: %s", m_mergefolder.c_str());
		// Retrieve the cache file and check for any changes
		std::string cache = m_mergefolder + "/.cache";
		fs::path cachepath = fs::u8path(cache);
		std::ofstream cachefile(cachepath.wstring(), std::ios::binary | std::ios::app);
		if (!cachefile) {
			logging::logwarning("FILELOADER::FileSettings::MergeFiles cannot cache filedata!\n%s", cache);
		}
		// Now comes the merging magic. maybe this can be optimized later on
		std::vector<RowInfo>&& data = m_parentFile->GetData();
		size_t dataSize = data.size();
		if (data.size() <= 0) {
			RowInfo emptyRow;
			for (auto& header : m_parentFile->GetHeaderNames()) {
				emptyRow.AddData(header, "empty");
			}
			data.push_back(emptyRow);
		}
		for (auto& path : m_mergefolderpaths) {
			FileInfo file;
			file.LoadFile(path);
			if (!file.IsReady()) {
				continue;
			}
			if (cachefile) {
				fs::path filep = fs::u8path(path);
				cachefile << path << " : " << GetLastWriteTime(filep) << "\n";
			}
			std::vector<RowInfo>&& mergeData = file.GetData();
			if (m_mergefolderif.first == "") {
				for (auto& row : mergeData) {
					if (dontimportvalues.size() > 0) {
						std::string value = row.GetData(m_dontimportifexistsheader);
						if (dontimportvalues.find(value) != dontimportvalues.end())
							continue;
					}
					// Setting up a new row
					RowInfo newrow = data.back();
					for (auto&& header : m_parentFile->GetHeaderNames()) {
						newrow.UpdateData(header, "");
					}
					// Getting and updating
					bool dataset = false;
					for (auto& mergeheader : m_mergeheadersfolder) {
						const std::string value = row.GetData(mergeheader.second);
						newrow.UpdateData(mergeheader.first, value);
						cellsImported++;
						dataset = true;
					}
					if (dataset) {
						m_parentFile->AddRowData(newrow);
					}
				}
			}
			else {
				int idx = -1;
				for (auto& row : data) {
					idx++;
					std::string value = row.GetData(m_mergefolderif.first);
					if (value == "")
						continue;
					for (auto& merge_row : mergeData) {
						std::string merge_value = merge_row.GetData(m_mergefolderif.second);
						if (merge_value == "")
							continue;
						if (merge_value != value)
							continue;
						for (auto& pair : m_mergeheadersfolder) {
							std::string new_val = merge_row.GetData(pair.second);
							if (new_val != "") {
								row.UpdateData(pair.first, new_val);
								cellsImported++;
							}
						}
						break;
					}
					if (row.Changed()) {
						m_parentFile->SetRowData(row, idx);
					}
				}
			}
			file.Unload();
		}
		SetMergeFolder(m_mergefolder);
	}
	if (!m_mergefile.IsReady())
		return;
	logging::loginfo("FILELOADER::FileSettings::MergeFiles Merging files\n\t%s\n\t%s\n\t And Searching for header: %s to fill with %s", m_parentFile->GetFilename().c_str(), m_mergefile.GetFilename().c_str(), m_mergeif.first.c_str(), m_mergeif.second.c_str());
	std::vector<RowInfo> &&data = m_parentFile->GetData();
	if (data.size() <= 0) {
		RowInfo emptyRow;
		for (auto& header : m_parentFile->GetHeaderNames()) {
			emptyRow.AddData(header, "empty");
		}
		data.push_back(emptyRow);
	}
	std::vector<RowInfo> &&mergeData = m_mergefile.GetData();
	int idx = -1;
	for (auto& row : data) {
		idx++;
		std::string value = row.GetData(m_mergeif.first);
		if (value == "")
			continue;
		for (auto& merge_row : mergeData) {
			std::string merge_value = merge_row.GetData(m_mergeif.second);
			if (merge_value == "")
				continue;
			if (merge_value != value)
				continue;
			for (auto& pair : m_mergeheaders) {
				std::string new_val = merge_row.GetData(pair.second);
				if (new_val != "") {
					row.UpdateData(pair.first, new_val);
					cellsImported++;
				}
			}
			break;
		}
		if (row.Changed()) {
			m_parentFile->SetRowData(row, idx);
		}
	}
	logging::loginfo("FILELOADER::FileSettings::MergeFiles %d Cells merged", cellsImported);
}

void FileSettings::SetMergeFolder(const std::string& folder, const bool ignoreCache) {
	fs::path path = fs::u8path(folder);
	m_mergefolderpaths.clear();

	try {
		if (!fs::exists(path)) {
			logging::loginfo("FILELOADER::FileSettings::SetMergeFolder Directory is not valid: %s", folder);
			return;
		}
		// read the cache file and ignore files that didnt change
		std::string cache = folder + "/.cache";
		fs::path cachepath = fs::u8path(cache);
		std::ifstream cachefile(cachepath.wstring(), std::ios::binary);
		std::vector<std::pair<std::string, std::string>> cachedData;
		std::vector<std::string> newcache;
		if (!ignoreCache && cachefile) {
			std::string line;
			while (std::getline(cachefile, line)) {
				RemoveAllSubstrings(line, "\n");
				std::pair<std::string, std::string> values = Splitlines(line, " : ");
				cachedData.push_back(values);
			}
		}
		// iterate each file and check for its ending to be a valid file
		for (const auto& entry : fs::directory_iterator(path)) {
			if (entry.is_regular_file()
				&& (entry.path().extension().string() == ".csv"
					|| entry.path().extension().string() == ".CSV"
					|| entry.path().extension().string() == ".xlsx"
					|| entry.path().extension().string() == ".XLSX")) {
				std::u8string u8str = entry.path().u8string();
				std::string strpath = std::string(u8str.begin(), u8str.end());
				for (auto& c : strpath) {
					if (c == '\\')
						c = '/';
				}
				bool add = true;
				for (auto& pair : cachedData) {
					// Dont add the file if the last writetime is same as in .cache
					if (pair.first == strpath && pair.second == GetLastWriteTime(entry.path())) {
						add = false;
						break;
					}
				}
				if (!add)
					continue;
				m_mergefolderpaths.insert(strpath);
			}
		}
		const std::string parentFname = fs::u8path(m_parentFile->GetFilename()).filename().string();
		logging::loginfo("FILELOADER::FileSettings::SetMergeFolder Files to merge: %d for File: %s", m_mergefolderpaths.size(), parentFname.c_str());
	}
	catch (const fs::filesystem_error& e) {
		logging::logerror("FIELELOADER::FileSettings::SetMergeFolder Filesystem error: \n%s", e.what());
		return;
	}
	catch (const std::exception& e) {
		std::cerr << "Other error: " << e.what() << '\n';
		logging::logerror("FILELOADER::FileSettings::SetMergeFolder Error: \n%s", e.what());
		return;
	}
	m_mergefolder = folder;
	m_mergefolderSet = true;
}

std::string FileSettings::GetMergeFolder() const{
	return m_mergefolder;
}

bool FileSettings::IsMergeFolderSet() const{
	return m_mergefolderSet;
}

std::unordered_set<std::string> FileSettings::GetMergeFolderPaths() const {
	return m_mergefolderpaths;
}

void FileSettings::SetMergeFolderTemplate(const std::string& filepath) {
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFolderTemplate m_parentFile is not set yet!");
	}
	if (m_mergefolderfile.IsReady()) {
		m_mergefolderif = {  };
		m_mergeheadersfolder.clear();
		m_mergefolderfile.Unload();
	}
	m_mergefolderfile.LoadFile(filepath);
	if (!m_mergefolderfile.IsReady()) {
		logging::logwarning("FILELOADER::FileSettings::SetMergeFolderTemplate could not load file properly: %s", filepath.c_str());
		m_mergefolderfile.Unload();
		m_mergefolderfile = FileInfo();
		return;
	}
	m_mergefolderfileSet = true;
}

FileInfo FileSettings::GetMergeFolderTemplate() const{
	return m_mergefolderfile;
}

bool FileSettings::IsMergeFolderTemplate() const{
	return m_mergefolderfileSet;
}

void FileSettings::AddFolderHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader) {
	if (destHeader != "" && !IsMergeFolderTemplate()) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_mergefoldertemplate is not set yet!");
		return;
	}
	if (!m_parentFile) {
		logging::logwarning("FILELOADER::FileSettings::AddHeaderToMerge m_parentFile is not set yet!");
		return;
	}
	for (auto& pair : m_mergeheadersfolder) {
		if (pair.first == sourceHeader) {
			pair.second = destHeader;
			return;
		}
	}
	m_mergeheadersfolder.push_back(std::make_pair(sourceHeader, destHeader));
}

void FileSettings::SetMergeFolderHeaderIf(const std::string& sourceHeader, const std::string& destHeader) {
	m_mergefolderif = std::make_pair(sourceHeader, destHeader);
}

void FileSettings::RemoveFolderHeaderToMerge(const std::string& header) {
	auto it = std::find_if(m_mergeheadersfolder.begin(), m_mergeheadersfolder.end(),
		[&header](const std::pair<std::string, std::string>& p) {
			return p.first == header;
		});
	// Check if the header even exists
	if (it == m_mergeheadersfolder.end()) {
		return;
	}
	m_mergeheadersfolder.erase(it);
}

std::pair<std::string, std::string> FileSettings::GetMergeFolderIf() const {
	return m_mergefolderif;
}
std::vector<std::pair<std::string, std::string>> FileSettings::GetMergeFolderHeaders() const {
	return m_mergeheadersfolder;
}

void FileSettings::SetDontImportIf(const std::string& header) {
	m_dontimportifexistsheader = header;
}

std::string FileSettings::GetDontImportIf() {
	return m_dontimportifexistsheader;
}
