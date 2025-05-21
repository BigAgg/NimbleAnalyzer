#pragma once

#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>

class RowInfo;
class FileSettings;
class FileInfo;

class FileInfo {
public:
	void LoadFile(const std::string& filename);
	void SaveFile();
	void SaveFileAs(const std::string& filename);
	void SaveFileAs(const std::string& sourcefile, const std::string& destfile);
	void CreateSheetData();
	std::string GetFilename() const;

	std::pair<int, int> GetHeaderIndex(const std::string& header);
	void GetHeaderIndex(const std::string& header, int* x, int* y);
	std::vector<std::string> GetHeaderNames() const;

	RowInfo GetRowdata(const int rowIdx);
	std::vector<RowInfo> GetData();
	void SetRowData(const RowInfo& rowinfo, const int rowIdx);
	void AddRowData(const RowInfo& rowinfo);

	bool IsReady() const;
	void Unload();

	FileSettings *Settings;

private:
	std::string m_filename = "";
	//										Header									Cell index
	std::vector<std::pair<std::string, std::pair<int, int>>> m_headerinfo;
	int m_headeridx = -1;
	std::vector<RowInfo> m_rowinfo;
	bool m_isready = false;
	std::vector<std::vector<std::string>> m_sheetData;
};

class RowInfo {
public:
	void AddData(const std::string& header, const std::string& value);
	void UpdateData(const std::string& header, const std::string& newValue);

	std::string GetData(const std::string& header) const ;
	std::vector<std::pair<std::string, std::string>> GetData() const;
	void SetData(const std::vector<std::pair<std::string, std::string>>& data);

	bool Changed();
	void ResetChanged();
	void Unload();

private:
	//										Header			 Value
	std::vector<std::pair<std::string, std::string>> m_rowinfo;
	bool m_changed = false;
};

class FileSettings {
public:
	void SetParentFile(FileInfo* parentFile);
	void SetMergeFile(const FileInfo otherFile);
	FileInfo GetMergeFile() const;
	std::pair<std::string, std::string> GetMergeIf();
	std::vector<std::pair<std::string, std::string>> GetMergeHeaders();
	void AddHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader);
	void SetMergeHeaderIf(const std::string& sourceHeader, const std::string& destHeader);
	void RemoveHeaderToMerge(const std::string& header);
	void MergeFiles();
	bool IsMergeFileSet() const;
	void SetMergeFolder(const std::string& folder);
	std::string GetMergeFolder() const;
	void Unload();
private:
	FileInfo* m_parentFile = nullptr;
	FileInfo m_mergefile;
	FileInfo m_mergefolderfile;
	std::string m_mergefolder = "";
	bool m_mergefileSet = false;
	std::vector<std::pair<std::string, std::string>> m_mergeheaders;
	std::pair<std::string, std::string> m_mergeif;
};

