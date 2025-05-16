#pragma once

#include "nfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <string>

inline std::string OpenDirectoryDialog() {
	namespace fs = std::filesystem;
	NFD_Init();
	nfdu8char_t* outPath;
	//nfdu8filteritem_t filters[0] = {};
	nfdpickfolderu8args_t args = { 0 };
	//nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
	nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
	fs::path outStr;
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
	return outStr.string();
	NFD_Quit();
}

inline std::string OpenFileDialog(const std::string& filterName, const std::string& fileEndings) {
	namespace fs = std::filesystem;
	NFD_Init();
	nfdu8char_t* outPath;
	nfdu8filteritem_t filters[1] = { {filterName.c_str(), fileEndings.c_str()} };
	nfdopendialogu8args_t args = { 0 };
	args.filterList = filters;
	args.filterCount = 1;
	nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
	std::string outStr = "";
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		if (!fs::exists(fs::u8path(outStr)))  // Use u8path for UTF-8
		{
			std::cerr << "Error: File does not exist: " << outStr << "\n";
		}
		else
		{
			std::cout << "Selected path: " << outStr << "\n";
		}
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		puts("User pressed cancel.");
	}
	else {
		printf("Error: %s\n", NFD_GetError());
	}
	NFD_Quit();
	for (char& c : outStr) {
		if (c == '\\') {
			c = '/';
		}
	}
	return outStr;
}

