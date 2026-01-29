cmake_minimum_required(VERSION 3.10)
project(screen-renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Fetch Dear ImGui
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Fetch ImGuiColorTextEdit
FetchContent_Declare(
  imgui_color_text_edit
  GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imgui_color_text_edit)

# Create ImGui library
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})

# Create ImGuiColorTextEdit library
add_library(ImGuiColorTextEditLib STATIC
  ${imgui_color_text_edit_SOURCE_DIR}/TextEditor.cpp
)
target_include_directories(ImGuiColorTextEditLib PUBLIC
  ${imgui_color_text_edit_SOURCE_DIR}
)
target_link_libraries(ImGuiColorTextEditLib PUBLIC imgui)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

# Source files
set(SOURCES
  src/main.cpp
  src/screen.cpp
  src/shader.cpp
  src/screen_renderer.cpp
  src/bitmap.cpp
  src/font.cpp
)

# Create executable
add_executable(screen-renderer ${SOURCES})

# Link libraries
target_link_libraries(screen-renderer
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
)

# Example programs
# Common library for examples
set(EXAMPLE_SOURCES
  src/screen.cpp
  src/shader.cpp
  src/screen_renderer.cpp
  src/bitmap.cpp
  src/font.cpp
  src/simple_markup.cpp
  src/markup_renderer.cpp
)

# Bitmap demo
add_executable(bitmap-demo
  examples/bitmap_demo.cpp
  ${EXAMPLE_SOURCES}
)
target_link_libraries(bitmap-demo
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
)

# Text demo
add_executable(text-demo
  examples/text_demo.cpp
  ${EXAMPLE_SOURCES}
)
target_link_libraries(text-demo
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
)

# Interactive demo with ImGui
add_executable(interactive-demo
  examples/interactive_demo.cpp
  ${EXAMPLE_SOURCES}
)
target_link_libraries(interactive-demo
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
  imgui
)

# Sensor status demo
add_executable(sensor-status-demo
  examples/sensor_status_demo.cpp
  ${EXAMPLE_SOURCES}
)
target_link_libraries(sensor-status-demo
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
)

# Styleable screen demo
add_executable(styleable-screen-demo
  examples/styleable_screen_demo.cpp
  ${EXAMPLE_SOURCES}
)
target_link_libraries(styleable-screen-demo
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
)

# Live Markup Editor
add_executable(live-markup-editor
  examples/live_markup_editor.cpp
  ${EXAMPLE_SOURCES}
)
target_include_directories(live-markup-editor PRIVATE
  ${imgui_color_text_edit_SOURCE_DIR}
)
target_link_libraries(live-markup-editor
  ${OPENGL_LIBRARIES}
  glfw
  GLEW::GLEW
  imgui
  ImGuiColorTextEditLib
)
