# Nimble Analyzer
**Nimble Analyzer** is a tool to read and process excel filedata (xlsx).

## Technical
It is written in C++ using [Raylib](https://www.raylib.com/), [ImGui](https://github.com/ocornut/imgui), [rlImGui](https://github.com/raylib-extras/rlImGui), [xlnt](https://github.com/tfussell/xlnt) and [nfd](https://github.com/btzy/nativefiledialog-extended).

## Cloning the repo:
```sh
git clone --recurse-submodules https://github.com/BigAgg/FlashLampAnalyzer.git
cd FlashLampAnalyzer
git submodule update --init --recursive
```

## Building the Analyzer
### Windows:
***Requirements***:
- [CMake](https://cmake.org/) Version 3.8 or higher
- Any c++ Compiler that supports c++20 or higher like gcc

#### Preparing the Project:
Simply run the *setup.sh* file. It installs all Dependencies and sets up the Project.\
If you are using **Visual Studio**, it generates a *.sln* inside the build directory that is ready to use.

#### Compiling and Building using Visual Studio:
Enter the *build* directory and type following command:
```sh
cmake --build . --config Release
```
The executable can now be found inside the *Release* directory.

#### Compiling and Building using Ninja:
Enter the *build* directory and type following command:
```sh
ninja
```
The executable can now be found inside the entered *build* directory


## Resources
[Raylib](https://www.raylib.com/)\
[ImGui](https://github.com/ocornut/imgui)\
[rlImGui](https://github.com/raylib-extras/rlImGui)\
[xlnt](https://github.com/tfussell/xlnt)\
[nfd](https://github.com/btzy/nativefiledialog-extended)

## License
[MIT](https://mit-license.org/)