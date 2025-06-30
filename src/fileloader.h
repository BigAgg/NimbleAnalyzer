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

#pragma once

#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>
#include <unordered_set>
// Splits all worksheets into separate .xlsx files
void SplitWorksheets(const std::string& filename);
void EditWorksheet(const std::string& filename, int DATA_row = 0, bool deleteEmptyRows = true);

// Class predefinitions
class RowInfo;
class FileSettings;
class FileInfo;

// FileInfo stores all data related to a excel file that can be loaded
class FileInfo {
public:
	// Load a file with given filename
	void LoadFile(const std::string& filename);
	// Save the loaded data to given filename
	void SaveFile(const std::string& filename = "");
	// Saves the loaded file as a given destfile and tries to load sourcefile if there is any
	void SaveFileAs(const std::string& sourcefile, const std::string& destfile);
	// Converts all RowInfo inside m_rowinfo into m_sheetData
	void CreateSheetData();
	// Returns the filename
	std::string GetFilename() const;
	
	// Get the index of a given header as a pair
	std::pair<int, int> GetHeaderIndex(const std::string& header);
	// Get the index of a given header as integers that are provided
	void GetHeaderIndex(const std::string& header, int* x, int* y);
	// Get the vector of all header names
	std::vector<std::string> GetHeaderNames() const;
	// Get whole headerinfo with information about header indexes inside m_sheetData
	std::vector<std::pair<std::string, std::pair<int, int>>> GetHeaderInfo();
	// Sets a header with info of given index inside m_sheetData
	void SetHeaderInfo(std::vector<std::pair<std::string, std::pair<int, int>>> headerinfo);
	
	// Gets the rowdate inside m_sheetData at given index
	RowInfo GetRowdata(const int rowIdx);
	// Gets the whole RowInfo data loaded
	std::vector<RowInfo> GetData();
	// Sets the RowInfo at a given row index
	void SetRowData(const RowInfo& rowinfo, const int rowIdx);
	// Adds RowInfo to the dataset
	void AddRowData(const RowInfo& rowinfo);
	// Removes RowInfo at given row index
	void RemoveData(const int rowIdx);
	
	// Returns if the fileInfo is read (file loaded)
	bool IsReady() const;
	// Unloads the file completely
	void Unload();
	
	// Loads the Settings that should apply tho this file
	void LoadSettings(const std::string& path);
	// Saves the Settings of the file to given path
	void SaveSettings(const std::string& path);

	// Pointer to owen Settings
	FileSettings *Settings;

private:
	std::string m_filename = "";
	//										Header									Cell index
	std::vector<std::pair<std::string, std::pair<int, int>>> m_headerinfo;	// whole information about headers and where they are located
	int m_headeridx = -1;	// Tells at what row the headers are in m_sheetData
	std::vector<RowInfo> m_rowinfo;	// Whole generated RowInfo data out of m_sheetData
	bool m_isready = false;	// bool that is set once the file is being loaded correctly
	std::vector<std::vector<std::string>> m_sheetData;	// loaded sheet
};

// RowInfo holds information of one Row inside a sheetData and can be used to access and modify data
class RowInfo {
public:
	// Adds a header with given value
	void AddData(const std::string& header, const std::string& value);
	// Updates a value of given Header
	void UpdateData(const std::string& header, const std::string& newValue);
	
	// Get the data of given header
	std::string GetData(const std::string& header) const ;
	// gets all data with header and value as vector
	std::vector<std::pair<std::string, std::string>> GetData() const;
	// Completely overwrite the data
	void SetData(const std::vector<std::pair<std::string, std::string>>& data);
	
	// A check, if the data was changed
	bool Changed();
	// Resets m_changed to false
	void ResetChanged();
	// Unloads all data
	void Unload();

private:
	//										Header			 Value
	std::vector<std::pair<std::string, std::string>> m_rowinfo;
	bool m_changed = false;
};

// All Settings that can be applied to a file
class FileSettings {
	// Maybe refactor everything since you dont really need alot of stuff due to mergefile and mergefolder sharing alot of
	// similarities
public:
	// Sets the file it is stored in, this is important to set before merging files
	void SetParentFile(FileInfo* parentFile);
	void SetMergeFile(const FileInfo otherFile);
	FileInfo GetMergeFile() const;
	std::pair<std::string, std::string> GetMergeIf() const;
	std::vector<std::pair<std::string, std::string>> GetMergeHeaders() const;
	std::pair<std::string, std::string> GetMergeFolderIf() const;
	std::vector<std::pair<std::string, std::string>> GetMergeFolderHeaders() const;
	void AddHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader);
	void SetMergeHeaderIf(const std::string& sourceHeader, const std::string& destHeader);
	void AddFolderHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader);
	void SetMergeFolderHeaderIf(const std::string& sourceHeader, const std::string& destHeader);
	void RemoveFolderHeaderToMerge(const std::string& header);
	void RemoveHeaderToMerge(const std::string& header);
	void MergeFiles();
	bool IsMergeFileSet() const;
	void SetMergeFolder(const std::string& folder, const bool ignoreCache = false);
	std::string GetMergeFolder() const;
	bool IsMergeFolderSet() const;
	std::unordered_set<std::string> GetMergeFolderPaths() const;
	void SetMergeFolderTemplate(const std::string& filepath);
	FileInfo GetMergeFolderTemplate() const;
	bool IsMergeFolderTemplate() const;
	void Unload();
	void SetDontImportIf(const std::string& header);
	std::string GetDontImportIf();
private:
	FileInfo* m_parentFile = nullptr;
	FileInfo m_mergefile;
	FileInfo m_mergefolderfile;
	std::string m_dontimportifexistsheader;
	std::unordered_set<std::string> m_mergefolderpaths;
	std::string m_mergefolder = "";
	bool m_mergefolderSet = false;
	bool m_mergefolderfileSet = false;
	bool m_mergefileSet = false;
	std::vector<std::pair<std::string, std::string>> m_mergeheadersfolder;
	std::pair<std::string, std::string> m_mergefolderif;
	std::vector<std::pair<std::string, std::string>> m_mergeheaders;
	std::pair<std::string, std::string> m_mergeif;
};

