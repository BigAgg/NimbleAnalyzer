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

	void HandleUI();
};