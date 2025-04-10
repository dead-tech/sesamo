cmake_minimum_required(VERSION 3.20.5)
project(sesamo)

include(FetchContent)

set(CXX_FLAGS "-std=c++2b -Wall -Wextra -Werror -Wshadow -Wconversion -Wpedantic -ggdb")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})

FetchContent_Declare(imgui_external
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.8.tar.gz
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_external)

add_library(imgui
	${imgui_external_SOURCE_DIR}/imgui.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR})

add_executable(${PROJECT_NAME} src/main.cpp src/Serial.cpp src/Application.cpp)
target_link_libraries(${PROJECT_NAME} glfw3 imgui GL)
