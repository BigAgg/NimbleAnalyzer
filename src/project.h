#pragma once

#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>

class Project;
class Data;
class Sheet;

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

private:
	std::string m_name = "";
	std::string m_parent = "";
	std::string m_currentFile = "";
	std::vector<std::string> m_paths;
};

class Data {
public:
	void SetValue(const std::string& identifier, const std::string& value);
	std::string GetValue(const std::string& identifier) const;
	void AddValue(const std::string& identifier, const std::string& value, xlnt::cell cell);
	xlnt::cell GetCellinfo(const std::string& identifier);

private:
	std::vector<std::pair<std::string, std::pair<std::string, xlnt::cell>>> m_cellinfo;
};

class Sheet {
public:
	void LoadFile(const std::string)
};
