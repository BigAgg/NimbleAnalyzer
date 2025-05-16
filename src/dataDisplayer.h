#pragma once

#include "fileloader.h"

enum DATA_DISPLAY_MODES {
	DATA_DISPLAY_MODE_NONE,
	DATA_DISPLAY_MODE_TOP_TO_BOT,
	DATA_DISPLAY_MODE_
};

void DisplayData(RowInfo &data, const std::string mode = "");

void DisplayDataWithSettings(RowInfo& data, std::vector<std::pair<std::string, bool>>& settings);
