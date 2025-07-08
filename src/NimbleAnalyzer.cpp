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

#include "NimbleAnalyzer.h"
#include <raylib.h>
#include <imgui.h>
#include "ui_helper.h"
#include <rlImGui.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#include "logging.h"
#include "project.h"
#include "fileDialog.h"
#include "fileloader.h"
#include "dataDisplayer.h"
#include "utils.h"

namespace fs = std::filesystem;

static Project new_project;
static Project *current_project = &new_project;
static int selected_project = -1;
static std::vector<Project> projects;

static void s_LoadProject(const std::string& name) {
	std::string projectName = Splitlines(name, "\\").second;	// Get the project name
	if (projectName == "")
		return;
	// Generate the project and load it
	projects.push_back(Project());
	projects.back().SetName(projectName);
	projects.back().Load(projectName);
}

static void s_LoadAllProjects() {
	std::vector<std::thread> threads;
	// Iterates through "projects/" and loads every project available
	for (const auto& dirEntry : fs::directory_iterator("projects")) {
		const std::u8string u8path = dirEntry.path().u8string();
		const std::string strpath(u8path.begin(), u8path.end());
		s_LoadProject(strpath);
	}
}

// Struct to store all window related settings
namespace engine {
	static struct {
		int windowW = 640;
		int windowH = 480;
		int fps = 30;
		bool maximized = false;
		int device = -1;
		int windowPosX = -1;
		int windowPosY = -1;
	} engineSettings;

	static ENGINE_ERROR errorcode = ENGINE_UNINITIALIZED_ERROR;

	static Image s_icon;	// Icon image that is loaded in Init()

	ENGINE_ERROR GetErrorcode() {
		return errorcode;
	}

	bool Init() {
		errorcode = ENGINE_NONE_ERROR;
		// Setting up needed directories
		fs::create_directory("bin");
		fs::create_directory("fonts");
		fs::create_directory("projects");
		fs::create_directory("sheets");
		fs::create_directory("backup");
		// Loading engine settings
		if (!LoadSettings()) {
			logging::logwarning("ENGINE::INIT Could not load settings, using default settings instead!");
		}
		// Initializing raylib
#ifdef NIMBLE_ANALYZER_VERSION
		std::string windowName = "NimbleAnalyzer ";
		windowName += NIMBLE_ANALYZER_VERSION;
#else
		std::string windowName = "NimbleAnalyzer";
#endif
		// Initialize Raylib and setup window
		InitWindow(engineSettings.windowW, engineSettings.windowH, windowName.c_str());
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		s_icon = LoadImage("NimbleAnalyzer.png");
		if (IsImageValid(s_icon))
			SetWindowIcon(s_icon);
		if (!IsWindowReady()) {
			logging::logerror("ENGINE::INIT Raylib could not be initialized!");
			errorcode = ENGINE_RAYLIB_ERROR;
			return false;
		}
		SetTargetFPS(engineSettings.fps);
		if (engineSettings.maximized)
			MaximizeWindow();
		// Setup monitor and window position
		if (engineSettings.device == -1 || engineSettings.device >= GetMonitorCount()) {
			engineSettings.device = GetCurrentMonitor();
			SetWindowMonitor(engineSettings.device);
			Vector2 windowPos = GetWindowPosition();
			if (windowPos.y < 0)
				windowPos.y = 0;
			engineSettings.windowPosX = static_cast<int>(windowPos.x);
			engineSettings.windowPosY = static_cast<int>(windowPos.y);
		}
		SetWindowMonitor(engineSettings.device);
		SetWindowPosition(engineSettings.windowPosX, engineSettings.windowPosY);
		SetExitKey(0);
		return true;
	}

	void Shutdown() {
		if (IsWindowMinimized())
			RestoreWindow();
		// Unload images
		UnloadImage(s_icon);
		// Updating current Monitor position to load window at same position
		engineSettings.device = GetCurrentMonitor();
		Vector2 windowPos = GetWindowPosition();
		if (windowPos.y < 0)
			windowPos.y = 0;
		engineSettings.windowPosX = static_cast<int>(windowPos.x);
		engineSettings.windowPosY = static_cast<int>(windowPos.y);
		// Saving settings to file
		if (!SaveSettings()) {
			logging::logerror("ENGINE::SHUTDOWN Settings could not be saved!");
			errorcode = ENGINE_SAVEFILE_ERROR;
		}
		// Closing window
		CloseWindow();
	}

	bool LoadSettings() {
		// Load settings from binary file into engineSettings struct
		std::ifstream file("bin/engine.bin", std::ios::binary);
		if (!file)
			return false;
		file.read((char*)&engineSettings, sizeof(engineSettings));
		return true;
	}

	bool SaveSettings() {
		// Saves engineSettings struct into binary file
		std::ofstream file("bin/engine.bin", std::ios::binary);
		if (!file)
			return false;
		file.write((char*)&engineSettings, sizeof(engineSettings));
		return true;
	}

	void Run() {
		// Main loop until window should close
		while (!WindowShouldClose()) {
			// Check if the window was resized last frame and adjust to it
			if (IsWindowResized()) {
				int width = GetScreenWidth();
				int height = GetScreenHeight();
				
				bool resize = false;
				if (width < 640) {
					width = 640;
					resize = true;
				}
				if (height < 480) {
					height = 480;
					resize = true;
				}
				if (resize)
					SetWindowSize(width, height);
				engineSettings.windowW = width;
				engineSettings.windowH = height;
			}
			Render();
		}
	}

	void Render() {
		// Skip drawing if the window is not even focused (cpu usage=0%)
		// Maybe add input detection aswell to only update when mouse movement
		// or key presses happened
		if (!IsWindowFocused()) {
			PollInputEvents();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return;
		}
		BeginDrawing();
		ClearBackground(BLACK);
		ui::HandleUI();
		EndDrawing();
	}
};

namespace ui {
	enum UI_MODE {
		UI_NONE,
		UI_PROJECT_WINDOW,
		UI_DATA_VIEW_WINDOW,
		UI_UPDATE_WINDOW,
		UI_DEFAULT = UI_PROJECT_WINDOW,
	};

	enum FILTER_MODE {
		FILTER_NONE,
		FILTER_GREATER_THAN,
		FILTER_LOWER_THAN,
		FILTER_OUT_OF_RANGE,
		FILTER_IN_RANGE,
		FILTER_EMPTY,
		FILTER_NOT_EMPTY,
		FILTER_MIN,
		FILTER_MAX,
		FILTERS,
		FILTER_DEFAULT = FILTER_NONE
	};

	// Textures that are used throughout the programm
	static Texture folder_icon;
	static Texture open_file_icon;
	static Texture file_icon;
	static Texture delete_file_icon;
	static Texture save_icon;
	static Texture save_as_icon;

	static struct {
		UI_MODE ui_mode = UI_DEFAULT;
		unsigned int ui_style = 0;
	} uiSettings;

	static UI_ERROR errorcode = UI_NONE_ERROR;

	UI_ERROR GetErrorcode() {
		return errorcode;
	}

	static bool s_updateAvail = false;
	static std::string s_changes;

	bool IsNewerVersion(const std::string& current, const std::string& available){
		std::istringstream cur(current), avail(available);
		int c1, c2;
		char dot;
		while (cur >> c1 && avail >> c2) {
			if (c1 < c2) return true;
			if (c1 > c2) return false;
			cur >> dot;
			avail >> dot;
		}
		return avail.rdbuf()->in_avail();	// if available has more parts
	}
	
	// Variables for filtering data and contents
	static std::vector<std::string> s_hiddenHeaders;
	static bool s_ignoreCache = false;
	static std::string s_viewmode = "horizontal-noheader";
	static std::string s_filter = "";
	static FILTER_MODE s_filtermode = FILTER_DEFAULT;
	static struct {
		float max = 0.0f;
		float min = 0.0f;
		std::string header = "";
	} filterSettings;
	static std::vector<std::pair<int, RowInfo>> s_filteredData;
	bool s_deleteEmptyLines = true;
	int s_rowDataPositionToAdd = 0;

	static std::string s_filterlist[FILTERS];

	bool Init() {
		// Check for newer version
#ifdef NIMBLE_ANALYZER_VERSION
		const std::string currVersion = NIMBLE_ANALYZER_VERSION;
		std::ifstream verFile("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/VERSION");
		if (verFile) {
			std::string availVersion;
			std::getline(verFile, availVersion);
			s_updateAvail = IsNewerVersion(currVersion, availVersion);
			if (s_updateAvail) {
				uiSettings.ui_mode = UI_UPDATE_WINDOW;
				std::ifstream changesFile("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/CHANGES");
				if (changesFile) {
					std::string line;
					while (std::getline(changesFile, line)) {
						s_changes += line + "\n";
					}
				}
			}
		}
#endif
		std::ifstream file("bin/ui.bin");
		if (file) {
			bool update = false;
			if (uiSettings.ui_mode == UI_UPDATE_WINDOW)
				update = true;
			file.read((char*)&uiSettings, sizeof(uiSettings));
			uiSettings.ui_mode = UI_DEFAULT;
			if (update)
				uiSettings.ui_mode = UI_UPDATE_WINDOW;
		}
		// initialize filterlist
		s_filterlist[FILTER_DEFAULT] = "Kein Filter";
		s_filterlist[FILTER_GREATER_THAN] = (char*)u8"Größer als";
		s_filterlist[FILTER_LOWER_THAN] = "Kleiner als";
		s_filterlist[FILTER_OUT_OF_RANGE] = (char*)u8"Außerhalb Toleranz";
		s_filterlist[FILTER_IN_RANGE] = "Innerhalb Toleranz";
		s_filterlist[FILTER_EMPTY] = "Leeres Feld";
		s_filterlist[FILTER_NOT_EMPTY] = (char*)u8"Ausgefülltes Feld";
		s_filterlist[FILTER_MIN] = "Niedrigster Wert";
		s_filterlist[FILTER_MAX] = (char*)u8"Höchster Wert";
		// Check if the engine is initialized
		if (engine::GetErrorcode() != engine::ENGINE_NONE_ERROR) {
			errorcode = UI_INIT_ERROR;
			logging::logerror("UI::INIT Engine is not initialized! Errorcode: %d", errorcode);
			return false;
		}
		// Loading icons
		delete_file_icon = LoadTexture("icons/delete_file_icon.png");
		if (!IsTextureValid(delete_file_icon))
			logging::logwarning("UI::INIT File Icon could not be found at './icons/delete_file_icon.png");
		save_as_icon = LoadTexture("icons/save_as_icon.png");
		if (!IsTextureValid(save_as_icon))
			logging::logwarning("UI::INIT File Icon could not be found at './icons/save_as_icon.png");
		file_icon = LoadTexture("icons/file_icon.png");
		if (!IsTextureValid(file_icon))
			logging::logwarning("UI::INIT File Icon could not be found at './icons/file_icon.png");
		open_file_icon = LoadTexture("icons/open_file_icon.png");
		if (!IsTextureValid(open_file_icon))
			logging::logwarning("UI::INIT File Icon could not be found at './icons/open_file_icon.png'");
		folder_icon = LoadTexture("icons/folder_icon.png");
		if (!IsTextureValid(folder_icon))
			logging::logwarning("UI::INIT Folder Icon could not be found at './icons/folder_icon.png'");
		save_icon = LoadTexture("icons/save_icon.png");
		if (!IsTextureValid(save_icon))
			logging::logwarning("UI::INIT Save Icon could not be found at './icons/save_icon.png'");
		// Initialize rlimgui
		rlImGuiSetup(false);
		ImGui::StyleColorsLight();
		// Setup custom font and
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMonoNerdFont-Bold.ttf", 18.0f);
		if (!font) {
			errorcode = UI_FONT_ERROR;
			logging::logerror("UI::INIT Font could not be loaded! Errorcode: %d", errorcode);
			logging::loginfo("UI::INIT Using Custom Font instead.");
		}
		else {
			io.FontDefault = font;
		}
		switch (uiSettings.ui_style) {
		case 1:
			ImGui::StyleColorsClassic();
			break;
		case 2:
			ImGui::StyleColorsDark();
			break;
		case 0:
		default:
			ImGui::StyleColorsLight();
			break;
		}
		// Remove imgui.ini
		io.IniFilename = nullptr;
		// Load Projects
		s_LoadAllProjects();
		return true;
	}

	void Shutdown() {
		// Save all projects loaded
		for (auto& project : projects) {
			project.Save();
		}
		// Unload all textures from the gpu
		UnloadTexture(folder_icon);
		UnloadTexture(open_file_icon);
		UnloadTexture(file_icon);
		UnloadTexture(delete_file_icon);
		UnloadTexture(save_icon);
		UnloadTexture(save_as_icon);
		rlImGuiShutdown();	// shutdown imgui
		std::ofstream file("bin/ui.bin");
		if (file) {
			file.write((char*)&uiSettings, sizeof(uiSettings));
		}
	}

	static void MainMenu() {
		ImGui::BeginMainMenuBar();
		// Arbeitsfenster wechseln
		if (ImGui::Button("Projekt wechseln")) {
			uiSettings.ui_mode = UI_DEFAULT;
			new_project = Project();
		}
		// Datenübersicht
		if (ImGui::Button((char*)u8"Datenübersicht")) {
			uiSettings.ui_mode = UI_DATA_VIEW_WINDOW;
		}
		// Error output
		if (ImGui::Button("Errors in txt")) {
			std::ofstream file("errors.txt");
			if (file) {
				for (auto& error : logging::GetErrors()) {
					file << error << "\n";
				}
			}
			std::ofstream _file("warnings.txt");
			if (_file) {
				for (auto& warning : logging::GetWarnings()) {
					_file << warning << "\n";
				}
			}
		}
		if (ImGui::BeginMenu("Dateieditor")) {
			if (ImGui::Button("Split Worksheets")) {
				const std::string filename = OpenFileDialog("Excel Sheet", "xlsx");
				if (filename != "") {
					std::string outputFolder = OpenDirectoryDialog();
					if (outputFolder == "")
						outputFolder = "sheets/";
					else
						outputFolder += "/";
					SplitWorksheets(filename, outputFolder);
				}
			}
			ImGui::SetItemTooltip("Konvertiert alle Tabellen zu einzelnen xlsx Dateien");
			if (ImGui::Button("Edit Worksheet")) {
				const std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
				if (filename != "") {
					EditWorksheet(filename, s_rowDataPositionToAdd, s_deleteEmptyLines);
				}
			}
			if (ImGui::Button("Edit Folder")) {
				const fs::path path = fs::u8path(OpenDirectoryDialog());
				if (path != "") {
					if (s_rowDataPositionToAdd > 0 || s_deleteEmptyLines && fs::exists(path)) {
						for (const auto& entry : fs::directory_iterator(path)) {
							if (!entry.is_regular_file())
								continue;

							const std::string fname = entry.path().string();

							// Process only xlsx files starting with "sheet_"
							if (StrEndswith(fname, ".xlsx") || StrEndswith(fname, ".csv")) {
								EditWorksheet(fname, s_rowDataPositionToAdd, s_deleteEmptyLines);
							}
						}
					}

				}
			}
			ImGui::SetItemTooltip((char*)u8"Bearbeitet gewählte Tabelle mit unten gesetzten Settings");
			if (ImGui::Checkbox("Leere Zeilen entfernen", &s_deleteEmptyLines));
			ImGui::SetItemTooltip("Entfernt aus allen gesplitteten Dateien leere Zeilen");
			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::InputInt((char*)u8"'DATA' in Reihe einfügen", &s_rowDataPositionToAdd));
			ImGui::SetItemTooltip((char*)u8"Wenn > 0 -> Fügt eine Reihe vor 'A' ein und fügt 'DATA' an\ngegebener Stelle ein (A:X)");
			ImGui::EndMenu();
		}
		if (ImGui::Button("Guide")) {
			std::string guidepath = "Nimble Analyzer Guide_ger.pdf";
			std::string command = "start \"\" \"" + guidepath + "\"";
			system(command.c_str());
		}
		ImGui::SetItemTooltip((char*)u8"Öffnet die NimbleAnalyzer Anleitung");

		if (ImGui::BeginMenu("Style")) {
			if (ImGui::Button("Dark")) {
				ImGui::StyleColorsDark();
				uiSettings.ui_style = 2;
			}
			if (ImGui::Button("Classic")) {
				ImGui::StyleColorsClassic();
				uiSettings.ui_style = 1;
			}
			if (ImGui::Button("Light")) {
				ImGui::StyleColorsLight();
				uiSettings.ui_style = 0;
			}
			ImGui::EndMenu();
		}
		if (ImGui::Button("Update")) {
			uiSettings.ui_mode = UI_UPDATE_WINDOW;
		}

		ImGui::EndMainMenuBar();
	}

	static void DisplayMenuBar() {
		// Drawing the main menu for this window
		ImGui::BeginMenuBar();
		// Create new Project
		if (ImGui::BeginMenu("Projekt anlegen")) {
			std::string name = new_project.GetName();
			if (ImGui::InputStringWithHint(name, "Projektname", "projektname")) {
				new_project.SetName(name);
			}
			if (ImGui::Button("Anlegen") && new_project.GetName() != "") {
				bool anlegen = true;
				// Check if the project name already exists
				for (const Project& p : projects) {
					if (p.GetName() == name) {
						anlegen = false;
						break;
					}
				}
				// Store the new project and select it, also save the previous selected project
				if (anlegen) {
					current_project->Save();
					projects.push_back(new_project);
					new_project = Project();
					current_project = &projects.back();
					selected_project = static_cast<int>(projects.size()) - 1;
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	static void DisplayProjectSelection() {
		// Listbox to select from the project list
		ImGui::Text((char*)u8"Projekt wählen");
		if (ImGui::BeginListBox("## Project selection", {300.0f, 75.0f})) {
			for (int x = 0; x < projects.size(); x++) {
				Project& project = projects[x];
				bool selected = (selected_project == x);
				const std::string& name = project.GetName();
				char buff[256];
				strncpy_s(buff, name.c_str(), 256);
				// If selection happened store the new one and save the old one
				if (ImGui::Selectable(buff, &selected)) {
					current_project->Save();
					selected_project = x;
					current_project = &projects[x];
					current_project->Load(name);
					s_hiddenHeaders.clear();
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
			// Project deletion
			if (projects.size() > 0 && ImGui::Button("Projekt entfernen") && selected_project >= 0) {
				current_project->Delete();
				current_project->Unload();
				projects.erase(projects.cbegin() + selected_project);
				selected_project -= 1;
				if (selected_project >= 0)
					current_project = &projects[selected_project];
				else
					current_project = &new_project;
			}
		}
	}

	static void DisplayFileSelection() {
		ImGui::Text((char*)u8"Projektdateien");
		// File selection for all files inside the selected project
		if (ImGui::BeginListBox("## File Selection", {400.0f, 75.0f})) {
			const std::vector<std::string> files = current_project->GetFilePaths();
			const std::string current_file = current_project->GetSelectedFile();
			int idx = 0;
			for (const std::string& file : files) {
				bool selected = (file == current_file);
				const fs::path p = file;
				char buff[256];
				strncpy_s(buff, (p.filename().string() + " ##" + std::to_string(idx)).c_str(), 256);
				if (ImGui::Selectable(buff, &selected)) {
					current_project->Save();
					current_project->SelectFile(file);
					current_project->loadedFile.Unload();
					current_project->loadedFile.LoadFile(file);
					fs::path tmpPath = fs::path(file);
					const std::string tmpstr = tmpPath.filename().string();
					const std::string projectName = current_project->GetName();
					current_project->loadedFile.LoadSettings("projects/" + projectName + "/" + tmpstr + ".ini");
					s_hiddenHeaders.clear();
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
				idx++;
			}
			ImGui::EndListBox();
		}
		// Handle adding new files
		if (rlImGuiImageButtonSize((char*)u8"Neue Datei Hinzufügen", &file_icon, { 30.0f, 30.0f })) {
			current_project->AddFilePath(OpenFileDialog("Excel Sheet", "xlsx,csv"));
		}
		ImGui::SetItemTooltip((char*)u8"Datei hinzufügen");
		ImGui::SameLine();
		// Delete files from project (selected file is getting deleted)
		if (rlImGuiImageButtonSize("Datei entfernen", &delete_file_icon, { 30.0f, 30.0f })) {
			current_project->RemoveFilePath(current_project->GetSelectedFile());
		}
		ImGui::SetItemTooltip("Datei entfernen");
		ImGui::SameLine();
		// Saving options for selected file
		if(rlImGuiImageButtonSize("Datei speichern als", &save_as_icon, {30.0f, 30.0f})) {
			const std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
			if (filename != "") {
				if (StrContains(filename, ".csv"))
					current_project->loadedFile.SaveFile(filename);
				else
					current_project->loadedFile.SaveFileAs(current_project->loadedFile.GetFilename(), filename);

			}
		}
		ImGui::SetItemTooltip("Datei speichern als");
		ImGui::SameLine();
		if (rlImGuiImageButtonSize("Datei speichern", &save_icon, { 30.0f, 30.0f })) {
			const std::string filename = current_project->loadedFile.GetFilename();
			BackupFile(filename);
			current_project->loadedFile.SaveFileAs(filename, filename);
		}
		ImGui::SetItemTooltip((char*)u8"Datei speichern (Überschreibt geladene Datei)");
	}

	static void DisplayFileSettings() {
		// Select mergefolder
		fs::path mergefolderpath = current_project->loadedFile.Settings->GetMergeFolder();
		if (rlImGuiImageButtonSize("Neuer Merge-Ordner", &folder_icon, { 30.0f, 30.0f })) {
			std::string folder = OpenDirectoryDialog();
			if (folder != "") {
				current_project->loadedFile.Settings->SetMergeFolder(folder, s_ignoreCache);
			}
		}
		ImGui::SetItemTooltip((char*)u8"Wähle einen neuen Merge-Ordner \n(Alle Dateien aus diesem Ordner werden in die aktuell ausgewählte Datei geladen)");
		ImGui::SameLine();
		ImGui::Text("Aktueller Merge-Ordner: %s", mergefolderpath.string().c_str());
		if (current_project->loadedFile.Settings->IsMergeFolderSet()) {
			if (rlImGuiImageButtonSize((char*)u8"Wähle template", &file_icon, {30.0f, 30.0f})) {
				std::string templatefile = OpenFileDialog("Excel Sheet", "xlsx,csv");
				if (templatefile != "") {
					current_project->loadedFile.Settings->SetMergeFolderTemplate(templatefile);
				}
			}
			ImGui::SetItemTooltip((char*)u8"Wähle Template");
			ImGui::SameLine();
			if (ImGui::Checkbox("Cache ignorieren", &s_ignoreCache)) {
				current_project->loadedFile.Settings->SetMergeFolder(current_project->loadedFile.Settings->GetMergeFolder(), s_ignoreCache);
			}
			ImGui::SetItemTooltip((char*)u8"Ignoriert die Cache Datei für diesen Merge-Ordner\nDas bedeutet, dass schon eingebundene Dateien erneut\nzum einbinden überprüft werden!");
			ImGui::SameLine();
			if (ImGui::Button((char*)u8"Cache löschen")) {
				const std::string folder = current_project->loadedFile.Settings->GetMergeFolder() + "/.cache";
				fs::path p = fs::u8path(folder);
				if (fs::exists(p)) {
					fs::remove(p);
					current_project->loadedFile.Settings->SetMergeFolder(current_project->loadedFile.Settings->GetMergeFolder(), s_ignoreCache);
				}
			}
		}
		// Select single mergefile
		fs::path filepath = current_project->loadedFile.Settings->GetMergeFile().GetFilename();
		ImGui::Text("Aktuelle Mergefile: %s", filepath.filename().string().c_str());
		if (rlImGuiImageButtonSize("Neue Mergefile", &file_icon, {30.0f, 30.0f})) {
			std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
			if (filename != "") {
				FileInfo mergefile;
				mergefile.LoadFile(filename);
				if (mergefile.IsReady()) {
					current_project->loadedFile.Settings->SetMergeFile(mergefile);
				}
			}
		}
		ImGui::SetItemTooltip((char*)u8"Neue Mergefile auswählen");
	}

	static void DisplayHeaderMergeSettings() {
		// Merging button and handling
		if (ImGui::Button("Daten Mergen")) {
			current_project->loadedFile.Settings->MergeFiles();
			s_ignoreCache = false;
		}
	
		// Retrieve data for displaying settings
		auto headers = current_project->loadedFile.GetHeaderNames();
		auto mergeheaders = current_project->loadedFile.Settings->GetMergeFile().GetHeaderNames();
		auto setmergeheaders = current_project->loadedFile.Settings->GetMergeHeaders();
		auto headerif = current_project->loadedFile.Settings->GetMergeIf();

		// Iterate each header that each RowInfo contains and display dropdown menus aswell as
		// Checkboxes to set their settings
		for (auto& header : headers) {
			if (header == "")
				continue;
			std::pair<std::string, std::string> setHeader = { "", "" };
			for (auto& pair : setmergeheaders) {
				if (pair.first == header) {
					setHeader = pair;
				}
			}
			std::string label = "## Datensuche ##" + header;
			bool searchif = (header == headerif.first);
			// Checkbox handles rather this header is to check if the value matches or not
			if (ImGui::Checkbox(label.c_str(), &searchif)) {
				if (searchif) {
					setHeader.first = header;
					current_project->loadedFile.Settings->SetMergeHeaderIf(setHeader.first, setHeader.second);
				}
				else {
					current_project->loadedFile.Settings->SetMergeHeaderIf("", "");
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(300.0f);
			// Combo for this header where you can select which header should be imported here
			if (ImGui::BeginCombo(header.c_str(), setHeader.second.c_str())) {
				for (auto& mergeheader : mergeheaders) {
					bool selected = (mergeheader == setHeader.second);
					if (ImGui::Selectable(mergeheader.c_str(), &selected)) {
						current_project->loadedFile.Settings->AddHeaderToMerge(header, mergeheader);
						if (header == headerif.first)
							current_project->loadedFile.Settings->SetMergeHeaderIf(header, mergeheader);
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			std::string reset_button = "Reset ## " + header;
			if (ImGui::Button(reset_button.c_str())) {
				current_project->loadedFile.Settings->RemoveHeaderToMerge(header);
			}
		}
	}

	static void DisplayHeaderSettings() {
		// Checkboxes to toggle certain values being displayed or not
		auto&& headers = current_project->loadedFile.GetHeaderNames();
		for (auto&& header : headers) {
			if (Splitlines(header, " ##").first == "")
				continue;
			if (header == "")
				continue;
			bool isSet = false;
			for (auto&& hiddenHeader : s_hiddenHeaders) {
				if (hiddenHeader == header) {
					isSet = true;
				}
			}
			if (ImGui::Checkbox(header.c_str(), &isSet)) {
				if (isSet) {
					bool add = true;
					for (auto&& hiddenHeader : s_hiddenHeaders) {
						if (hiddenHeader == header) {
							add = false;
							break;
						}
					}
					if (add)
						s_hiddenHeaders.push_back(header);
				}
				else {
					auto it = std::find(s_hiddenHeaders.begin(), s_hiddenHeaders.end(), header);
					if (it != s_hiddenHeaders.end()) {
						s_hiddenHeaders.erase(it);
					}
				}
			}
		}
	}

	static void DisplayHeaderMergeFolderSettings() {
		// Retrieve data
		auto headers = current_project->loadedFile.GetHeaderNames();
		auto mergeheaders = current_project->loadedFile.Settings->GetMergeFolderTemplate().GetHeaderNames();
		auto setmergeheaders = current_project->loadedFile.Settings->GetMergeFolderHeaders();
		auto headerif = current_project->loadedFile.Settings->GetMergeFolderIf();
		std::string dontimportif = current_project->loadedFile.Settings->GetDontImportIf();

		// Top menubar
		ImGui::BeginMenuBar();
		// merging data
		if (ImGui::Button("Daten Mergen")) {
			current_project->loadedFile.Settings->MergeFiles();
			s_ignoreCache = false;
		}
		// Combo to select a header, this header is being ignored when importing data
		// which means that if this headers value already exists in the sourcefile
		// it doesnt get imported again
		ImGui::Text("Daten Ignorieren wenn Header");
		if (ImGui::BeginCombo("## Header ignorieren", dontimportif.c_str())) {
			bool noneselect = (dontimportif == "NONE");
			if (ImGui::Selectable("NONE", &noneselect)) {
				current_project->loadedFile.Settings->SetDontImportIf("NONE");
			}
			for (auto& header : headers) {
				if (Splitlines(header, " ##").first == "")
					continue;
				bool selected = (header == dontimportif);
				if (ImGui::Selectable(header.c_str(), &selected)) {
					current_project->loadedFile.Settings->SetDontImportIf(header);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndMenuBar();
		
		// Simply the header selection same as in function above where we have a combo
		// To select import headers foreach source header
		for (auto& header : headers) {
			if (header == "")
				continue;
			std::pair<std::string, std::string> setHeader = { "", "" };
			for (auto& pair : setmergeheaders) {
				if (pair.first == header) {
					setHeader = pair;
				}
			}
			std::string label = "## Datensuche ##" + header;
			bool searchif = (header == headerif.first);
			if (ImGui::Checkbox(label.c_str(), &searchif)) {
				if (searchif) {
					setHeader.first = header;
					current_project->loadedFile.Settings->SetMergeFolderHeaderIf(setHeader.first, setHeader.second);
				}
				else {
					current_project->loadedFile.Settings->SetMergeFolderHeaderIf("", "");
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(300.0f);
			if (ImGui::BeginCombo(header.c_str(), setHeader.second.c_str())) {
				for (auto& mergeheader : mergeheaders) {
					bool selected = (mergeheader == setHeader.second);
					if (ImGui::Selectable(mergeheader.c_str(), &selected)) {
						current_project->loadedFile.Settings->AddFolderHeaderToMerge(header, mergeheader);
						if (header == headerif.first)
							current_project->loadedFile.Settings->SetMergeFolderHeaderIf(header, mergeheader);
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			std::string reset_button = "Reset ## " + header;
			if (ImGui::Button(reset_button.c_str())) {
				current_project->loadedFile.Settings->RemoveFolderHeaderToMerge(header);
			}
		}
	}

	static void ProjectWindow() {
		// Setup flags and settings
		int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
		int flags_nohscroll = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar;
		int flags_nomenu = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
		float screenW = static_cast<float>(GetScreenWidth());
		float screenH = static_cast<float>(GetScreenHeight());
		// Generate and setup the window
		ImGui::SetNextWindowSize({ screenW, screenH - 22.0f });
		ImGui::SetNextWindowPos({ 0.0f, 22.0f });
		ImGui::Begin("Wareneingang ## Window", nullptr, flags_nohscroll);
		DisplayMenuBar();
		// Drawing Project selection
		ImGui::BeginChild("Project selection window", {300.0f, 170.0f}, 0, flags_nomenu);
		DisplayProjectSelection();
		ImGui::EndChild();
		// Drawing File selection
		if (current_project->GetName() != "") {
			ImGui::SameLine();
			ImGui::BeginChild("Project file selection window", {500.0f, 170.0f}, 0, flags_nomenu);
			DisplayFileSelection();
			ImGui::EndChild();
			// Drawing file settings
			if (current_project->loadedFile.IsReady()) {
				ImGui::SameLine();
				ImGui::BeginChild("File settings window", { 500.0f, 170.0f }, 0, flags_nomenu);
				DisplayFileSettings();
				ImGui::EndChild();
				ImGui::BeginChild("Header settings", { 300.0f, 250.0f }, 0, flags_nomenu);
				ImGui::SeparatorText("Werte ausblenden");
				DisplayHeaderSettings();
				ImGui::EndChild();
				// Diplay merging settings if mergefile is loaded
				if (current_project->loadedFile.Settings->GetMergeFile().IsReady()) {
					ImGui::SameLine();
					ImGui::BeginChild("Header merge settings window", { 700.0f, 250.0f }, 0, flags_nomenu);
					ImGui::SeparatorText((char*)u8"Merge header wählen");
					DisplayHeaderMergeSettings();
					ImGui::EndChild();
				}
				// Display merge folder settings if mergefolder is set
				if (current_project->loadedFile.Settings->IsMergeFolderSet()
					&& current_project->loadedFile.Settings->GetMergeFolderTemplate().IsReady()) {
					ImGui::SameLine();
					ImGui::BeginChild("Header folder merge settings window", { 700, 250.0f }, 0, flags);
					DisplayHeaderMergeFolderSettings();
					ImGui::EndChild();
				}
			}
		}
		std::vector<std::string> consolelog = logging::GetAllMessages();
		std::string mergedlog = "";
		for (const std::string& log : consolelog) {
			mergedlog = log + "\n" + mergedlog;
		}
		// Maybe delete this as it is just for debugging? idk
		ImGui::BeginChild("console", { screenW, screenH - 600 }, 0, flags_nomenu);
		ImGui::InputTextMultiline("## Changes_Input", mergedlog.data(), mergedlog.capacity() + 1, {0, 0}, ImGuiInputTextFlags_ReadOnly);
		/*if (consolelog.size() > 0) {
			for (int x = consolelog.size() - 1; x >= 0; x--) {
				std::string label = consolelog[x] + " ##" + std::to_string(x);
				if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
					if (ImGui::IsMouseDoubleClicked(0)) {
						ImGui::SetClipboardText(consolelog[x].c_str());
					}
				}
			}
		}*/
		ImGui::EndChild();
		ImGui::End();
	}

	void DataViewWindow() {
		if (!current_project->loadedFile.IsReady()) {
			uiSettings.ui_mode = UI_DEFAULT;
			return;
		}
		// Collecting the dataset
		std::vector<RowInfo>&& data = current_project->loadedFile.GetData();
		auto&& headers = current_project->loadedFile.GetHeaderNames();
		// Setting up window flaghs and settings
		int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
		int flags_nomenu = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
		float screenW = static_cast<float>(GetScreenWidth());
		float screenH = static_cast<float>(GetScreenHeight());
		ImGui::SetNextWindowPos({ 0.0f, 22.0f });
		ImGui::SetNextWindowSize({ static_cast<float>(screenW), static_cast<float>(screenH) - 22.0f });
		// Beginning the window
		ImGui::Begin((char*)u8"Datenübersicht", nullptr, flags);
		// Mainmenu of this window
		ImGui::BeginMenuBar();
		if (ImGui::BeginMenu("Ansicht")) {
			if (ImGui::Button("Horizontal Einzel Header")) {
				s_viewmode = "horizontal-noheader";
			}
			if (ImGui::Button("Horizontal")) {
				s_viewmode = "horizontal-aboveheader";
			}
			if (ImGui::Button("Vertikal R")) {
				s_viewmode = "vertical-rightheader";
			}
			if (ImGui::Button("Vertikal L")) {
				s_viewmode = "vertical-leftheader";
			}
			ImGui::EndMenu();
		}
		// Dropdwon to select which headers not to display
		if (ImGui::BeginMenu("Header Ausblenden")) {
			DisplayHeaderSettings();
			ImGui::EndMenu();
		}
		// Create a new row dataset at the bottom
		if (ImGui::Button((char*)u8"Neuen Datensatz einfügen")) {
			RowInfo rinfo;
			for (auto& header : headers) {
				rinfo.AddData(header, "");
			}
			current_project->loadedFile.AddRowData(rinfo);
		}
		if (ImGui::Button((char*)u8"Datensätze löschen")) {
			current_project->loadedFile.ClearData();
		}
		if (ImGui::BeginMenu("Filteroptionen")) {
			// Reset filters
			if (ImGui::Button((char*)u8"Filter zurücksetzen")) {
				s_filter = "";
				s_filteredData.clear();
				filterSettings.header = "";
				filterSettings.max = 0.0f;
				filterSettings.min = 0.0f;
			}
			// save filtered data to file
			if (ImGui::Button("Gefilterte Daten exportieren") && s_filteredData.size() > 0) {
				const std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
				if (filename != "") {
					FileInfo saveFile;
					saveFile.SetHeaderInfo(current_project->loadedFile.GetHeaderInfo());
					for (std::pair<int, RowInfo> pair : s_filteredData) {
						saveFile.AddRowData(pair.second);
					}
					saveFile.SaveFile(filename);
					saveFile.Unload();
				}
			}
			// Search for filter
			if (ImGui::InputStringWithHint(s_filter, "Filter", "stichwort")) {
				s_filteredData.clear();
				// first = index inside data, second = actual RowInfo
				const std::vector<std::string> headernames = current_project->loadedFile.GetHeaderNames();
				for (int x = 0; x < data.size(); x++) {
					RowInfo &row = data[x];
					bool hasFilter = false;
					for (const std::string& header : headernames) {
						const std::string value = row.GetData(header);
						if (StrContains(value, s_filter)) {
							hasFilter = true;
							break;
						}
					}
					if (hasFilter) {
						s_filteredData.push_back(std::make_pair(x, row));
					}
				}
			}
			// Mathematical filter options
			ImGui::SeparatorText("Mathematische Filteroptionen");
			if (ImGui::BeginCombo("Option", s_filterlist[s_filtermode].c_str())) {
				for (int x = 0; x < FILTERS; x++) {
					const std::string fl = s_filterlist[x];
					bool selected = (x == s_filtermode);
					if (ImGui::Selectable(fl.c_str(), &selected)) {
						s_filtermode = static_cast<FILTER_MODE>(x);
						s_filteredData.clear();
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			// Header selection to apply filter to
			if (ImGui::BeginCombo("Header filtern", filterSettings.header.c_str())) {
				bool selected = (filterSettings.header == "NONE" || filterSettings.header == "");
				if (ImGui::Selectable("NONE", &selected))
					filterSettings.header = "NONE";
				if (selected)
					ImGui::SetItemDefaultFocus();
				for (auto&& header : headers) {
					if (Splitlines(header, " ##").first == "")
						continue;
					selected = (header == filterSettings.header);
					if (ImGui::Selectable(header.c_str(), &selected))
						filterSettings.header = header;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			// Now apply the different filter methods (create different functions for it?)
			// Definetly needs a refactor i think but its working fine rn
			switch (s_filtermode) {
			case FILTER_MIN:
				if (ImGui::Button("Filter anwenden")) {
					float value_min;
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo& rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (x == 0) {
							value_min = value_number;
							s_filteredData.push_back(std::make_pair(x, rinfo));
							continue;
						}
						if (value_min > value_number) {
							s_filteredData.clear();
							value_min = value_number;
							s_filteredData.push_back(std::make_pair(x, rinfo));
						}
						else if (value_min == value_number) {
							s_filteredData.push_back(std::make_pair(x, rinfo));
						}
					}
				}
				break;
			case FILTER_MAX:
				if (ImGui::Button("Filter anwenden")) {
					float value_max;
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo& rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (x == 0) {
							value_max = value_number;
							s_filteredData.push_back(std::make_pair(x, rinfo));
							continue;
						}
						if (value_max < value_number) {
							s_filteredData.clear();
							value_max = value_number;
							s_filteredData.push_back(std::make_pair(x, rinfo));
						}
						else if (value_max == value_number) {
							s_filteredData.push_back(std::make_pair(x, rinfo));
						}
					}
				}
				break;
			case FILTER_GREATER_THAN:
				if (ImGui::InputFloat("Max", &filterSettings.max)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number > filterSettings.max)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				break;
			case FILTER_LOWER_THAN:
				if (ImGui::InputFloat("Min", &filterSettings.min)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number < filterSettings.min)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				break;
			case FILTER_OUT_OF_RANGE:
				if (ImGui::InputFloat("Min", &filterSettings.min)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number < filterSettings.min || value_number > filterSettings.max)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				if (ImGui::InputFloat("Max", &filterSettings.max)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number < filterSettings.min || value_number > filterSettings.max)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				break;
			case FILTER_IN_RANGE:
				if (ImGui::InputFloat("Min", &filterSettings.min)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number > filterSettings.min && value_number < filterSettings.max)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				if (ImGui::InputFloat("Max", &filterSettings.max)) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo &rinfo = data[x];
						std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							continue;
						else if (!IsNumber(value) && !IsInteger(value))
							continue;
						ReplaceAllSubstrings(value, ",", ".");
						float value_number = std::stof(value);
						if (value_number > filterSettings.min && value_number < filterSettings.max)
							s_filteredData.push_back(std::make_pair(x,rinfo));
					}
				}
				break;
			case FILTER_EMPTY:
				if (ImGui::Button("Filter Anwenden")) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo& rinfo = data[x];
						const std::string value = rinfo.GetData(filterSettings.header);
						if (value == "")
							s_filteredData.push_back(std::make_pair(x, rinfo));
					}
				}
				break;
			case FILTER_NOT_EMPTY:
				if (ImGui::Button("Filter Anwenden")) {
					s_filteredData.clear();
					for (int x = 0; x < data.size(); x++) {
						RowInfo& rinfo = data[x];
						const std::string value = rinfo.GetData(filterSettings.header);
						if (value != "")
							s_filteredData.push_back(std::make_pair(x, rinfo));
					}
				}
				break;
			case FILTER_NONE:
			default:
				if (s_filteredData.size() > 0 && s_filter == "")
					s_filteredData.clear();
				break;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
		// Dataview drawing headers if needed
		if (s_viewmode == "horizontal-noheader") {
			ImGui::BeginChild("headers", {(DEFAULT_INPUT_WIDTH + 10.0f) * (headers.size() - s_hiddenHeaders.size()) + 50.0f, 25.0f});
			ImGui::Button(" X ");
			ImGui::SameLine();
			int idx = 0;
			for (auto&& header : headers) {
				auto it = std::find(s_hiddenHeaders.begin(), s_hiddenHeaders.end(), header);
				if (it != s_hiddenHeaders.end())
					continue;
				std::string splitheader = Splitlines(header, " ##").first;
				if (splitheader == "") {
					continue;
				}
				if(header != headers.front() && idx > 0)
					ImGui::SameLine();
				ImGui::SetNextItemWidth(DEFAULT_INPUT_WIDTH);
				ImGui::InputString(splitheader, "##" + header, ImGuiInputTextFlags_ReadOnly);
				ImGui::SetItemTooltip(splitheader.c_str());
				idx++;
			}
			ImGui::EndChild();
		}
		ImGui::Separator();
		ImGui::BeginChild("dataview", {(DEFAULT_INPUT_WIDTH + 10.0f) * (headers.size() - s_hiddenHeaders.size()) + 50.0f, screenH - 125.0f}, 0, flags_nomenu);
		// Now drawing the filtered data if there is any
		if (s_filteredData.size() == 0 && s_filter == "") {
			int x = 0;
			for (RowInfo& row : data) {
				ImGui::SetNextItemWidth(6.0f);
				if (ImGui::Button((" X ##" + std::to_string(x)).c_str())) {
					current_project->loadedFile.RemoveData(x);
				}
				ImGui::SetItemTooltip((char*)u8"Löscht diesen kompletten Eintrag!");
				if(StrContains(s_viewmode, "horizontal"))
					ImGui::SameLine();
				DisplayData(data[x], x, s_viewmode, s_hiddenHeaders);
				if (row.Changed())
					current_project->loadedFile.SetRowData(row, x);
				x++;
			}
		}
		// If no filtered, simply draw the whole dataset available
		else {
			for (std::pair<int, RowInfo> pair : s_filteredData) {
				ImGui::SetNextItemWidth(6.0f);
				if (ImGui::Button((" X ##" + std::to_string(pair.first)).c_str())) {
					current_project->loadedFile.RemoveData(pair.first);
				}
				ImGui::SetItemTooltip((char*)u8"Löscht diesen kompletten Eintrag!");
				if(StrContains(s_viewmode, "horizontal"))
					ImGui::SameLine();
				DisplayData(pair.second, pair.first, s_viewmode, s_hiddenHeaders);
				if (pair.second.Changed()) {
					current_project->loadedFile.SetRowData(pair.second, pair.first);
				}
			}
		}
		ImGui::EndChild();
		ImGui::End();
	}
	static void UpdateWindow() {
		float screenW = static_cast<float>(GetScreenWidth());
		float screenH = static_cast<float>(GetScreenHeight());
		ImGui::SetNextWindowPos({ 0.0f, 22.0f });
		ImGui::SetNextWindowSize({ static_cast<float>(screenW), static_cast<float>(screenH) - 22.0f });
		ImGui::Begin("Update Window", nullptr);
		ImGui::Text("New Update available");
		if (ImGui::Button("Update")) {
			std::string installerPath = "Y:\\Produktion\\Software & Tools\\NimbleAnalyzer\\src\\output\\setup_NimbleAnalyzer.exe";
			fs::copy_file(installerPath, ".\\installer.exe", fs::copy_options::overwrite_existing);
			std::string batPath = "update_temp.bat";
			std::string appPath = fs::current_path().string() + "\\installer.exe";
			std::ofstream bat(batPath);
			bat << "@echo off\n";
			bat << "timeout /t 2 /nobreak >nul\n"; // wait for 2 seconds
			bat << "copy /Y \"" << installerPath << "\" \"" << appPath << "\"\n";
			bat << "start \"\" \"" << appPath << "\"\n";
			bat << "del \"%~f0\"\n"; // delete the script itself
			bat.close();
			system(("start " + batPath).c_str());
			CloseWindow();
		}
		ImGui::SeparatorText("Changes");
		ImGui::InputTextMultiline("## Changes_Input", s_changes.data(), s_changes.capacity() + 1, {0, 0}, ImGuiInputTextFlags_ReadOnly);
		ImGui::End();
	}

	void HandleUI() {
		rlImGuiBegin();

		MainMenu();
		
		switch (uiSettings.ui_mode) {
		case UI_PROJECT_WINDOW:
			ProjectWindow();
			break;
		case UI_DATA_VIEW_WINDOW:
			DataViewWindow();
			break;
		case UI_UPDATE_WINDOW:
			UpdateWindow();
			break;
		case UI_NONE:
		default:
			uiSettings.ui_mode = UI_DEFAULT;
			break;
		}

		rlImGuiEnd();
	}
};