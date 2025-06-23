#pragma once

#include <string>
#include <vector>
#include "fileloader.h"

class Project;

class Project {
public:
	void SetName(const std::string& name);
	std::string GetName() const;

	void AddFilePath(const std::string& path);
	void RemoveFilePath(const std::string& path);
	std::vector<std::string> GetFilePaths() const;

	void LoadAllFileData();
	void LoadFileData(const std::string& path);

	void SelectFile(const std::string& path);
	std::string GetSelectedFile() const;

	void Unload();

	void Load(const std::string& name);
	void Save();
	void Delete();

	FileInfo loadedFile;

private:
	std::string m_name = "";					// Project name
	std::string m_currentFile = "";		// To know which file is currently active
	std::vector<std::string> m_paths;	// All paths added to the project
};

