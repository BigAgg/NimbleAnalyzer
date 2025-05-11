#include "project.h"

void Project::SetName(const std::string& name){
	m_name = name;
}

std::string Project::GetName() const{
	return m_name;
}

void Project::SetParent(const std::string& parent){
	m_parent = parent;
}

std::string Project::GetParent() const{
	return m_parent;
}

void Project::AddFilePath(const std::string& path){
	if (path == "")
		return;
	for (std::string& m_path : m_paths)
		if (m_path == path)
			return;
	m_paths.resize(m_paths.size() + 1);
	m_paths[m_paths.size() - 1] = path;
}

void Project::RemoveFilePath(const std::string& path){
	m_paths.erase(std::find(m_paths.begin(), m_paths.end(), path));
}

std::vector<std::string> Project::GetFilePaths() const{
	return m_paths;
}

void Project::LoadAllFileData(){
}

void Project::LoadFileData(const std::string& path){
}

void Project::SelectFile(const std::string& path){
	for (std::string& s : m_paths) {
		if (path == s) {
			m_currentFile = path;
			return;
		}
	}
}

std::string Project::GetSelectedFile() const{
	return m_currentFile;
}

void Data::SetValue(const std::string& identifier, const std::string& value) {
	for (auto& p : m_cellinfo) {
		if (p.first == identifier) {
			p.second.first = value;
			return;
		}
	}
}

std::string Data::GetValue(const std::string& identifier) const{
	for (auto& p : m_cellinfo) {
		if (p.first == identifier)
			return p.second.first;
	}
	return "";
}

void Data::AddValue(const std::string& identifier, const std::string& value, xlnt::cell cell){
	for (auto& p : m_cellinfo) {
		if (p.first == identifier)
			return;
	}
	m_cellinfo.push_back(std::make_pair(identifier, std::make_pair(value, cell)));
}

xlnt::cell Data::GetCellinfo(const std::string& identifier){
	for (auto& p : m_cellinfo) {
		if (p.first == identifier)
			return p.second.second;
	}
	xlnt::cell cell = *new xlnt::cell(cell);
	return cell;
}
