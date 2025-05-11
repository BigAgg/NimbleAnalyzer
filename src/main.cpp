#include "NimbleAnalyzer.h"

int main(int argc, char *argv[]) {
  if (!engine::Init())
    return engine::GetErrorcode();
  if (!ui::Init())
    return ui::GetErrorcode();
  engine::Run();
  ui::Shutdown();
  engine::Shutdown();
  return 0;
}
