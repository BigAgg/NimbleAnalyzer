#pragma once

#include "nfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <string>

inline std::string OpenDirectoryDialog() {
	NFD_Init();
	nfdu8char_t* outPath;
	//nfdu8filteritem_t filters[0] = {};
	nfdpickfolderu8args_t args = { 0 };
	//nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
	nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
	std::string outStr;
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		puts(outPath);
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		outStr = "";
	}
	else {
		outStr = "";
	}
	return outStr;
	NFD_Quit();
}

inline std::string OpenFileDialog(const std::string& filtername, const std::string& filter) {
	NFD_Init();
	nfdu8char_t* outPath;
	nfdu8filteritem_t filters[1] = { {filtername.c_str(), filter.c_str()} };
	nfdopendialogu8args_t args = { 0 };
	args.filterList = filters;
	args.filterCount = 1;
	nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
	std::string outStr;
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		outStr = "";
	}
	else {
		outStr = "";
	}
	return outStr;
	NFD_Quit();
}

