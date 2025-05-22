cmake_minimum_required(VERSION 3.20)
project(NimbleAnalyzer)
# Project Version 
set(PROJECT_VERSION "0.0.1")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Setting compile definitions
add_compile_definitions(VERSION="${PROJECT_VERSION}")

# Add raylib
add_subdirectory(external/raylib)
# Add nfd
add_subdirectory(external/nfd)
# Add xlnt
set(XLNT_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(external/xlnt)

# Add ImGui
add_library(ImGui STATIC
  external/imgui/imgui.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_demo.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
)
target_include_directories(ImGui PUBLIC
  external/imgui
  external/imgui/backends
)

# Add rlImGui
add_library(rlImGui STATIC
  external/rlimgui/rlImGui.cpp
)
target_include_directories(rlImGui PUBLIC
  external/rlimgui
)
target_link_libraries(rlImGui PRIVATE raylib ImGui)

# File organization (for visual studio)
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c) 
file(GLOB_RECURSE HEADER_FILES src/*.hpp src/*.h)

source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

# Building target 
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
# Setup buildflags
if(WIN32)
  if(MSVC)
    # For Visual Studio (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
    )
  else()
    # For GCC/MinGW on Windows
    target_link_options(${PROJECT_NAME} PRIVATE
      "$<$<CONFIG:Release>:-mwindows>"
    )
  endif()
endif()
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib ImGui rlImGui nfd xlnt)

# Check if Generator is Visual Studio 
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  message(STATUS "Setting up for Visual Studio")
  # Set the working Directory for each CONFIG 
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# set the source directory of resources
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)

# Copy the contents of the res folder to the build directory if there is one
if(EXISTS ${RES_DIR})
  message(STATUS "Resource folder fond: ${RES_DIR}")
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying resources to the build directory..."
  )
else()
  # Skipping copying resources
  message(STATUS "Resource folder not found: ${RES_DIR}. Skipping sesource copy.")
endif()

# Copy xlnt.dll after build (only if xlnt is built as a shared library)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:xlnt>  # This gets the .dll or .so depending on platform
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMENT "Copying xlnt.dll to the output directory..."
)
