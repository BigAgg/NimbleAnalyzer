#pragma once

#include <string>
#include <vector>
#include "fileloader.h"

class Project;

class Project {
public:
	void SetName(const std::string& name);
	std::string GetName() const;
	void SetParent(const std::string& parent);
	std::string GetParent() const;

	void AddFilePath(const std::string& path);
	void RemoveFilePath(const std::string& path);
	std::vector<std::string> GetFilePaths() const;

	void LoadAllFileData();
	void LoadFileData(const std::string& path);

	void SelectFile(const std::string& path);
	std::string GetSelectedFile() const;

	void Unload();

	FileInfo loadedFile;

private:
	std::string m_name = "";
	std::string m_parent = "";
	std::string m_currentFile = "";
	std::vector<std::string> m_paths;
};

