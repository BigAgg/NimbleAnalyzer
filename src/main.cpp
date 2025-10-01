#include "NimbleAnalyzer.h"
#include "logging.h"

int main(int argc, char *argv[]) {
#ifdef NDEBUG
  logging::startlogging("", "run.log"); // log to file in Release mode
#endif
  bool error = false;
  if (!engine::Init())
    return engine::GetErrorcode();
  if (!ui::Init())
    return ui::GetErrorcode();
  try {
		engine::Run();
  }
  catch (std::exception& e) {
    logging::logerror("MAIN Programm crashed: %s", e.what());
    error = true;
  }
  try {
		ui::Shutdown();
		engine::Shutdown();
  }
  catch (std::exception& e) {
    logging::logwarning(e.what());
  }
#ifdef NDEBUG
  logging::stoplogging();
#endif
  if(error)
    engine::ErrorWindow();
  return 0;
}

// This is needed when building in Release with msvc
#ifdef _MSC_VER
#ifdef NDEBUG
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  return main(__argc, __argv);
}
#endif
#endif
