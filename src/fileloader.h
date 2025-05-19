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
	std::string GetFilename() const;

	std::pair<int, int> GetHeaderIndex(const std::string& header);
	void GetHeaderIndex(const std::string& header, int* x, int* y);
	std::vector<std::string> GetHeaderNames() const;

	RowInfo GetRowdata(const int rowIdx);
	std::vector<RowInfo> GetData();
	void SetRowData(const RowInfo& rowinfo, const int rowIdx);
	void AddRowData(const RowInfo& rowinfo);

	bool IsReady() const;

	FileSettings *Settings;

private:
	std::string m_filename = "";
	//										Header									Cell index
	std::vector<std::pair<std::string, std::pair<int, int>>> m_headerinfo;
	std::vector<RowInfo> m_rowinfo;
	bool m_isready = false;
	std::vector<std::vector<std::string>> m_sheetData;
};

class RowInfo {
public:
	void AddData(const std::string& header, const std::string& value);
	void UpdateData(const std::string& header, const std::string& newValue);

	std::string GetData(const std::string& header) const ;
	std::vector<std::pair<std::string, std::string>> GetData();
	void SetData(const std::vector<std::pair<std::string, std::string>>& data);

	bool Changed();
	void ResetChanged();

private:
	//										Header			 Value
	std::vector<std::pair<std::string, std::string>> m_rowinfo;
	bool m_changed = false;
};

enum FILE_HEADER_SETTING {
	FILE_HEADER_SETTING_NONE,
	FILE_HEADER_SETTING_INVISIBLE,
	FILE_HEADER_SETTING_IMUTABLE,
	FILE_HEADER_SETTING_DATE,
};

enum FILE_SETTING {
	FILE_SETTING_NONE,
	FILE_SETTING_IMUTABLE,
	FILE_SETTING_OVERWRITE,
};

class FileSettings {
public:
	void AddHeaderSetting(const std::string& header);
	void EditHeaderSetting(const std::string& header, const FILE_HEADER_SETTING setting, const bool state);
	bool GetHeaderSetting(const std::string& header, const FILE_HEADER_SETTING setting) const;
	void SetFileSetting(const FILE_SETTING setting);
	bool ToggleFileSetting(const FILE_SETTING setting);
	bool GetFileSetting(const FILE_SETTING setting) const;

	void SetParentFile(FileInfo* parentFile);
	void SetMergeFile(const FileInfo otherFile);
	FileInfo GetMergeFile() const;
	void AddHeaderToMerge(const std::string& sourceHeader, const std::string& destHeader);
	void SetMergeHeaderIf(const std::string& sourceHeader, const std::string& destHeader);
	void RemoveHeaderToMerge(const std::string& header);
	void MergeFiles();
	bool IsMergeFileSet() const;
private:
	void m_initbool(bool* container, size_t container_size);
	std::vector<std::pair<std::string, bool[sizeof(FILE_HEADER_SETTING)]>> m_headersettings;
	bool m_filesettings[sizeof(FILE_SETTING)];
	bool m_initialized = false;
	FileInfo* m_parentFile = nullptr;
	FileInfo m_mergefile;
	bool m_mergefileSet = false;
	std::vector<std::pair<std::string, std::string>> m_mergeheaders;
	std::pair<std::string, std::string> m_mergeif;
};

