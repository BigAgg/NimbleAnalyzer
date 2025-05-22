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
	if (path == "")
		return;
	m_paths.erase(std::find(m_paths.begin(), m_paths.end(), path));
	if (path == m_currentFile) {
		m_currentFile = "";
		loadedFile.Unload();
	}
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

std::string Project::GetSelectedFile() const {
	return m_currentFile;
}

void Project::Unload() {
	if (loadedFile.IsReady())
		loadedFile.Unload();
	m_name = "";
	m_parent = "";
	m_currentFile = "";
	m_paths.clear();
}
