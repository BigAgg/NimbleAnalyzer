#pragma once

namespace engine {
	enum ENGINE_ERROR {
		ENGINE_NONE_ERROR,
		ENGINE_UNINITIALIZED_ERROR,
		ENGINE_RAYLIB_ERROR,
		ENGINE_SAVEFILE_ERROR,
		ENGINE_LOADFILE_ERROR,
		ENGINE_RUNTIME_ERROR,
		ENGINE_LAST_ERROR = ENGINE_RUNTIME_ERROR
	};


	ENGINE_ERROR GetErrorcode();
	bool Init();
	void Shutdown();
	bool LoadSettings();
	bool SaveSettings();

	void Run();
	void Render();
};

namespace analyzer {
	//static Project loadedProject;
};

namespace ui {
	enum UI_ERROR {
		UI_NONE_ERROR = engine::ENGINE_LAST_ERROR + 1,
		UI_INIT_ERROR,
		UI_FONT_ERROR,
		UI_RENDER_ERROR,
		UI_LAST_ERROR = UI_RENDER_ERROR
	};

	UI_ERROR GetErrorcode();
	bool Init();
	void Shutdown();
	bool LoadSettings();
	bool SaveSettings();

	void HandleUI();
};