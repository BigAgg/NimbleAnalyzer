#include "NimbleAnalyzer.h"

#include <raylib.h>
#include <imgui.h>
#include "ui_helper.h"
#include <rlImGui.h>
#include <filesystem>
#include <fstream>
#include <vector>

#include "logging.h"
#include "project.h"
#include "fileDialog.h"
#include "fileloader.h"
#include "dataDisplayer.h"

namespace fs = std::filesystem;

static Project new_project;
static Project *current_project = &new_project;
static std::vector<Project> projects;
static int selected_project = -1;

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

	ENGINE_ERROR GetErrorcode() {
		return errorcode;
	}

	bool Init() {
		errorcode = ENGINE_NONE_ERROR;
		// Setting up needed directories
		fs::create_directory("bin");
		fs::create_directory("backup");
		fs::create_directory("fonts");
		// Loading engine settings
		if (!LoadSettings()) {
			logging::logwarning("ENGINE::INIT Could not load settings, using default settings instead!");
		}
		// Initializing raylib
		InitWindow(engineSettings.windowW, engineSettings.windowH, "NimbleAnalyzer");
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		if (!IsWindowReady()) {
			logging::logerror("ENGINE::INIT Raylib could not be initialized!");
			errorcode = ENGINE_RAYLIB_ERROR;
			return false;
		}
		SetTargetFPS(engineSettings.fps);
		if (engineSettings.maximized)
			MaximizeWindow();

		if (engineSettings.device == -1) {
			engineSettings.device = GetCurrentMonitor();
			Vector2 windowPos = GetWindowPosition();
			if (windowPos.y < 0)
				windowPos.y = 0;
			engineSettings.windowPosX = static_cast<int>(windowPos.x);
			engineSettings.windowPosY = static_cast<int>(windowPos.y);
		}
		SetWindowMonitor(engineSettings.device);
		SetWindowPosition(engineSettings.windowPosX, engineSettings.windowPosY);
		return true;
	}

	void Shutdown() {
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
		std::ifstream file("bin/engine.bin", std::ios::binary);
		if (!file)
			return false;
		file.read((char*)&engineSettings, sizeof(engineSettings));
		return true;
	}

	bool SaveSettings() {
		std::ofstream file("bin/engine.bin", std::ios::binary);
		if (!file)
			return false;
		file.write((char*)&engineSettings, sizeof(engineSettings));
		return true;
	}

	void Run() {
		while (!WindowShouldClose()) {
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
		BeginDrawing();
		ClearBackground(BLACK);
		ui::HandleUI();
		EndDrawing();
	}
};

namespace analyzer {

};

namespace ui {
	enum UI_MODE {
		UI_NONE,
		UI_PROJECT_WINDOW,
		UI_LOADED_PROJECT_WINDOW,
		UI_DEFAULT = UI_PROJECT_WINDOW,
	};
	
	static Texture file_icon;
	static Texture folder_icon;
	static Texture save_icon;

	static struct {
		UI_MODE ui_mode = UI_DEFAULT;
	} uiSettings;

	static UI_ERROR errorcode = UI_NONE_ERROR;

	UI_ERROR GetErrorcode() {
		return errorcode;
	}

	static std::vector<std::string> s_hiddenHeaders;

	bool Init() {
		// Check if the engine is initialized
		if (engine::GetErrorcode() != engine::ENGINE_NONE_ERROR) {
			errorcode = UI_INIT_ERROR;
			logging::logerror("UI::INIT Engine is not initialized! Errorcode: %d", errorcode);
			return false;
		}
		// Loading icons
		file_icon = LoadTexture("icons/file_icon.png");
		if (!IsTextureValid(file_icon))
			logging::logwarning("UI::INIT File Icon could not be found at './icons/file_icon.png'");
		folder_icon = LoadTexture("icons/folder_icon.png");
		if (!IsTextureValid(folder_icon))
			logging::logwarning("UI::INIT Folder Icon could not be found at './icons/folder_icon.png'");
		save_icon = LoadTexture("icons/save_icon.png");
		if (!IsTextureValid(save_icon))
			logging::logwarning("UI::INIT Save Icon could not be found at './icons/save_icon.png'");
		// Initialize rlimgui
		rlImGuiSetup(false);
		ImGui::StyleColorsLight();
		// Setup custom font
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
		// Loading settings
		if (!LoadSettings()) {
			logging::logwarning("UI::INIT Settings could not be loaded!");
			logging::loginfo("UI::INIT Using default settings instead.");
		}
		return true;
	}

	void Shutdown() {
		if (!SaveSettings()) {
			logging::logwarning("UI::SHUTDOWN Settings could not be saved!");
			logging::loginfo("UI::SHUTDOWN Deleting './bin/ui.bin' in case it got corrupted.");
			std::remove("bin/ui.bin");
		}
		rlImGuiShutdown();
	}

	bool LoadSettings() {
		std::ifstream file("bin/ui.bin", std::ios::binary);
		if (!file)
			return false;
		file.read((char*)&uiSettings, sizeof(uiSettings));
		return true;
	}

	bool SaveSettings() {
		std::ofstream file("bin/ui.bin", std::ios::binary);
		if (!file)
			return false;
		file.write((char*)&uiSettings, sizeof(uiSettings));
		return true;
	}

	static void MainMenu() {
		ImGui::BeginMainMenuBar();
#ifdef VERSION
		ImGui::Text("%s", VERSION);
#endif
		// Arbeitsfenster wechseln
		if (ImGui::Button("Projekt wechseln")) {
			uiSettings.ui_mode = UI_DEFAULT;
			new_project = Project();
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
				for (const Project& p : projects) {
					if (p.GetName() == name) {
						anlegen = false;
						break;
					}
				}
				if (anlegen) {
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
		ImGui::Text((char*)u8"Projekt wählen");
		if (ImGui::BeginListBox("## Project selection", {300.0f, 75.0f})) {
			for (int x = 0; x < projects.size(); x++) {
				Project& project = projects[x];
				bool selected = (selected_project == x);
				const std::string& name = project.GetName();
				char buff[256];
				strncpy_s(buff, name.c_str(), 256);
				if (ImGui::Selectable(buff, &selected)) {
					selected_project = x;
					current_project = &projects[x];
					s_hiddenHeaders.clear();
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
			if (projects.size() > 0 && ImGui::Button("Projekt entfernen")) {
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
		if (ImGui::BeginListBox("## File Selection", {400.0f, 75.0f})) {
			const std::vector<std::string> files = current_project->GetFilePaths();
			const std::string current_file = current_project->GetSelectedFile();
			for (const std::string& file : files) {
				bool selected = (file == current_file);
				const fs::path p = file;
				char buff[256];
				strncpy_s(buff, p.filename().string().c_str(), 256);
				if (ImGui::Selectable(buff, &selected)) {
					current_project->SelectFile(file);
					current_project->loadedFile.Unload();
					current_project->loadedFile.LoadFile(file);
					s_hiddenHeaders.clear();
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		if (ImGui::Button((char*)u8"Neue Datei Hinzufügen")) {
			current_project->AddFilePath(OpenFileDialog("Excel Sheet", "xlsx,csv"));
		}
		if(ImGui::Button("Datei speichern als")){
			const std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
			if (filename != "")
				current_project->loadedFile.SaveFileAs(current_project->loadedFile.GetFilename(), filename);
		}
	}

	static void DisplayFileSettings() {
		// Select mergefolder
		fs::path mergefolderpath = current_project->loadedFile.Settings->GetMergeFolder();
		ImGui::Text("Aktueller Mergefolder: %s", mergefolderpath.string().c_str());
		if (ImGui::Button("Neuer Mergefolder")) {
			std::string folder = OpenDirectoryDialog();
			if (folder != "") {
				current_project->loadedFile.Settings->SetMergeFolder(folder);
			}
		}
		if (current_project->loadedFile.Settings->IsMergeFolderSet()) {
			ImGui::SameLine();
			if (ImGui::Button((char*)u8"Wähle template")) {
				std::string templatefile = OpenFileDialog("Excel Sheet", "xlsx,csv");
				if (templatefile != "") {
					current_project->loadedFile.Settings->SetMergeFolderTemplate(templatefile);
				}
			}
		}
		// Select single mergefile
		fs::path filepath = current_project->loadedFile.Settings->GetMergeFile().GetFilename();
		ImGui::Text("Aktuelle Mergefile: %s", filepath.filename().string().c_str());
		if (ImGui::Button("Neue Mergefile")) {
			std::string filename = OpenFileDialog("Excel Sheet", "xlsx,csv");
			if (filename != "") {
				FileInfo mergefile;
				mergefile.LoadFile(filename);
				if (mergefile.IsReady()) {
					current_project->loadedFile.Settings->SetMergeFile(mergefile);
				}
			}
		}
	}

	static void DisplayHeaderMergeSettings() {
		if (ImGui::Button("Daten Mergen")) {
			current_project->loadedFile.Settings->MergeFiles();
		}

		auto headers = current_project->loadedFile.GetHeaderNames();
		auto mergeheaders = current_project->loadedFile.Settings->GetMergeFile().GetHeaderNames();
		auto setmergeheaders = current_project->loadedFile.Settings->GetMergeHeaders();
		auto headerif = current_project->loadedFile.Settings->GetMergeIf();

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
					current_project->loadedFile.Settings->SetMergeHeaderIf(setHeader.first, setHeader.second);
				}
				else {
					current_project->loadedFile.Settings->SetMergeHeaderIf("", "");
				}
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(300.0f);
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
		auto&& headers = current_project->loadedFile.GetHeaderNames();
		for (auto&& header : headers) {
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

	static void ProjectWindow() {
		int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
		float screenW = static_cast<float>(GetScreenWidth());
		float screenH = static_cast<float>(GetScreenHeight());
		ImGui::SetNextWindowSize({ screenW, screenH - 22.0f });
		ImGui::SetNextWindowPos({ 0.0f, 22.0f });
		ImGui::Begin("Wareneingang ## Window", nullptr, flags);
		DisplayMenuBar();
		// Drawing Project selection
		ImGui::BeginChild("Project selection window", {300.0f, 170.0f});
		DisplayProjectSelection();
		ImGui::EndChild();
		// Drawing File selection
		if (current_project->GetName() != "") {
			ImGui::SameLine();
			ImGui::BeginChild("Project file selection window", {500.0f, 170.0f});
			DisplayFileSelection();
			ImGui::EndChild();
			// Drawing file settings
			if (current_project->loadedFile.IsReady()) {
				ImGui::SameLine();
				ImGui::BeginChild("File settings window", { 500.0f, 170.0f });
				DisplayFileSettings();
				ImGui::EndChild();
				ImGui::BeginChild("Header settings", { 300.0f, 250.0f });
				ImGui::SeparatorText("Werte ausblenden");
				DisplayHeaderSettings();
				ImGui::EndChild();
				// Diplay merging settings if mergefile is loaded
				ImGui::SameLine();
				if (current_project->loadedFile.Settings->GetMergeFile().IsReady()) {
					ImGui::BeginChild("Header merge settings window", { 700.0f, 250.0f });
					ImGui::SeparatorText((char*)u8"Merge header wählen");
					DisplayHeaderMergeSettings();
					ImGui::EndChild();
				}
			}
		}
		// Drawing the data for testing
		if (current_project->loadedFile.Settings) {
			ImGui::Separator();
			std::vector<RowInfo>&& data = current_project->loadedFile.GetData();
			int x = 0;
			for (RowInfo& row : data) {
				//DisplayData(row, x, "horizontal-aboveheader");
				DisplayData(row, x, "horizontal-aboveheader", s_hiddenHeaders);
				if (row.Changed())
					current_project->loadedFile.SetRowData(row, x);
				x++;
			}
		}
		ImGui::End();
	}

	void HandleUI() {
		rlImGuiBegin();

		MainMenu();
		
		switch (uiSettings.ui_mode) {
		case UI_PROJECT_WINDOW:
			ProjectWindow();
			break;
		case UI_NONE:
		default:
			uiSettings.ui_mode = UI_DEFAULT;
			break;
		}

		rlImGuiEnd();
	}
};