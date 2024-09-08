cmake_minimum_required(VERSION 3.12)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set architecture flags for M3
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Specify paths for libraries (adjust if necessary)
set(HOMEBREW_PREFIX "/opt/homebrew")
set(GLFW_ROOT_DIR "${HOMEBREW_PREFIX}/opt/glfw")
set(GLEW_ROOT_DIR "${HOMEBREW_PREFIX}/opt/glew")
set(GLM_ROOT_DIR "${HOMEBREW_PREFIX}/opt/glm")
set(FFTW3_ROOT_DIR "${HOMEBREW_PREFIX}/opt/fftw")
set(PORTAUDIO_ROOT_DIR "${HOMEBREW_PREFIX}/opt/portaudio")
set(SNDFILE_ROOT_DIR "${HOMEBREW_PREFIX}/opt/libsndfile")

# Include directories
include_directories(
    ${GLFW_ROOT_DIR}/include
    ${GLEW_ROOT_DIR}/include
    ${GLM_ROOT_DIR}/include
    ${FFTW3_ROOT_DIR}/include
    ${PORTAUDIO_ROOT_DIR}/include
    ${SNDFILE_ROOT_DIR}/include
)

# Link directories
link_directories(
    ${GLFW_ROOT_DIR}/lib
    ${GLEW_ROOT_DIR}/lib
    ${FFTW3_ROOT_DIR}/lib
    ${PORTAUDIO_ROOT_DIR}/lib
    ${SNDFILE_ROOT_DIR}/lib
)

# Find required packages
find_package(OpenGL REQUIRED)

# Add ImGui source files
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)
if(NOT EXISTS ${IMGUI_DIR})
    message(FATAL_ERROR "ImGui directory not found. Please clone ImGui to ${IMGUI_DIR}")
endif()

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add source files and create the executable target
add_executable(AudioVisualizer
    src/main.cpp
    src/audio_processor.cpp
    src/visualizer.cpp
    src/file_dialog_mac.mm 
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(AudioVisualizer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(AudioVisualizer PRIVATE
    ${OPENGL_LIBRARIES}
    "-framework OpenGL"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework AudioUnit"
    "-framework CoreServices"
    "-framework Cocoa"
    glfw
    GLEW
    fftw3f
    portaudio
    sndfile
)
# Set Objective-C++ compilation for the file dialog
set_source_files_properties(src/file_dialog_mac.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")

# Copy shader files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})