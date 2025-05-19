#include "NimbleAnalyzer.h"
#include "logging.h"

int main(int argc, char *argv[]) {
  logging::startlogging("", "run.log");
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
  logging::stoplogging();
  return 0;
}
