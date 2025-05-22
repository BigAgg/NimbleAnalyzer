#include "NimbleAnalyzer.h"
#include "logging.h"
#include <Windows.h>

int main(int argc, char *argv[]) {
  if (!engine::Init())
    return engine::GetErrorcode();
  if (!ui::Init())
    return ui::GetErrorcode();
  try {
		engine::Run();
  }
  catch (std::exception& e) {
    logging::logerror("MAIN Programm crashed: %s", e.what());
  }
	ui::Shutdown();
	engine::Shutdown();
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  return main(__argc, __argv);
}
