#pragma once

/*
* dataDisplay serves as simple tool to display and edit
* RowInfo from fileloader.h
*/

#include "fileloader.h"
#include <vector>

#define DEFAULT_INPUT_WIDTH 175.0f

enum DATA_DISPLAY_MODES {
	DATA_DISPLAY_MODE_NONE,
	DATA_DISPLAY_MODE_TOP_TO_BOT,
	DATA_DISPLAY_MODE_
};

// Displays a single RowInfo with a given mode
void DisplayData(RowInfo &data, const unsigned int identifier, const std::string &mode = "vertical-rightheader", const std::vector<std::string>& hiddenHeaders = std::vector<std::string>());
// Displays a set of RowInfo with a given mode
void DisplayDataset(std::vector<RowInfo>& data, const std::string& mode = "vertical-rightheader", const std::vector<std::string>& hiddenHeaders = std::vector<std::string>());

