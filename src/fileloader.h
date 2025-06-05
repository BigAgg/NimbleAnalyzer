#pragma once

#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>
#include <unordered_set>

class RowInfo;
class FileSettings;
class FileInfo;

class FileInfo {
public:
	void LoadFile(const std::string& filename);
	void SaveFile(const std::string& filename = "");
	void SaveFileAs(const std::string& sourcefile, const std::string& destfile);
	void CreateSheetData();
	std::string GetFilename() const;

	std::pair<int, int> GetHeaderIndex(const std::string& header);
	void GetHeaderIndex(const std::string& header, int* x, int* y);
	std::vector<std::string> GetHeaderNames() const;
	std::vector<std::pair<std::string, std::pair<int, int>>> GetHeaderInfo();
	void SetHeaderInfo(std::vector<std::pair<std::string, std::pair<int, int>>> headerinfo);

	RowInfo GetRowdata(const int rowIdx);
	std::vector<RowInfo> GetData();
	void SetRowData(const RowInfo& rowinfo, const int rowIdx);
	void AddRowData(const RowInfo& rowinfo);
	void RemoveData(const int rowIdx);

	bool IsReady() const;
	void Unload();

	void LoadSettings(const std::string& path);
	void SaveSettings(const std::string& path);

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

