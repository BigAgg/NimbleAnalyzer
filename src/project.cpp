#include "project.h"
#include "logging.h"
#include "utils.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void Project::SetName(const std::string& name){
	m_name = name;
}

std::string Project::GetName() const{
	return m_name;
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
	m_currentFile = "";
	m_paths.clear();
}

void Project::Load(const std::string& name) {
	m_name = name;
	fs::path propath = fs::path("projects") / name;
	if (!fs::exists(propath)) {
		logging::logwarning("PROJECT::Project::Load Project does not exist: name");
		return;
	}
	std::ifstream file(propath.string() + "/.pro", std::ios::binary);
	if (!file) {
		logging::logwarning("PROJECT::Project::Load no .pro file existing");
		return;
	}
	std::string line;
	std::getline(file, line);
	RemoveAllSubstrings(line, "\n");
	m_currentFile = line;
	std::getline(file, line);
	RemoveAllSubstrings(line, "\n");
	const int amount = std::stoi(line);
	m_paths.clear();
	for (int x = 0; x < amount; x++) {
		std::getline(file, line);
		RemoveAllSubstrings(line, "\n");
		if(line != "")
			m_paths.push_back(line);
	}
}

void Project::Save() {
	if (m_name == "") {
		return;
	}
	fs::create_directory("projects/" + m_name);
	std::ofstream file("projects/" + m_name + "/" + ".pro", std::ios::binary);
	if (file) {
		file << m_currentFile << '\n';
		file << m_paths.size() << '\n';
		for (const std::string& path : m_paths) {
			file << path << '\n';
		}
	}
	loadedFile.SaveSettings("projects/" + m_name + "/");
}

void Project::Delete() {
	if (m_name == "") {
		return;
	}
	fs::path projectPath = fs::path("projects") / m_name;

	// Prevent deleting root
	if (projectPath == "projects") {
		return;
	}
	// Checking if exists and deleting with all contents
	if (fs::exists(projectPath)) {
		fs::remove_all(projectPath);
	}
}
