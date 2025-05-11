cmake_minimum_required(VERSION 3.8)
project(NimbleAnalyzer)
# Project Version
set(PROJECT_VERSION "0.0.1")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Set other flags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Organize source files into "Source Files"
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h")

source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

# Setting compile definitions
add_compile_definitions(VERSION="${PROJECT_VERSION}")
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# Adding libraries
# Raylib
add_subdirectory(external/raylib)
target_include_directories(raylib PUBLIC
  external/raylib/src
)
# nfd
add_subdirectory(external/nfd)
# xlnt
set(XLNT_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(external/xlnt)
# rlimgui
add_library(rlImGui STATIC
  external/rlimgui/rlImGui.cpp
)
target_include_directories(rlImGui PUBLIC
  external/rlImGui
  external/imgui
)
target_link_libraries(rlImGui PRIVATE raylib)

# Add source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib rlImGui xlnt nfd)

# Check if Generator is Visual studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  message(STATUS "Setting up for Visual Studio...")
  # Set the working Directory for each CONFIG
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
elseif(CMAKE_GENERATOR MATCHES "Ninja")
  if(WIN32)
    # Copy compile_commands.json
    add_custom_target(
      copy_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Copying compile_commands.json to source directory (windows)"
      VERBATIM
    ) 
  else()
    # symlink compile_commands.json
    add_custom_target(
      copy_compile_commands ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
      COMMENT "Symlinking compile_commands.json to source directory (Unix)"
      VERBATIM
    )
  endif()
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
  message(WARNING "Resource folder not found: ${RES_DIR}. Skipping sesource copy.")
endif()


