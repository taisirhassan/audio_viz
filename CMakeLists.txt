cmake_minimum_required(VERSION 3.12)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(EXTRA_LIBS
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreServices"
        "-framework Cocoa"
    )
    
    # Set Homebrew prefix for M1 Macs
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
    
    # Add Homebrew include and lib directories
    include_directories(${HOMEBREW_PREFIX}/include)
    link_directories(${HOMEBREW_PREFIX}/lib)
elseif(WIN32)
    # Windows specific settings
    set(EXTRA_LIBS opengl32)
else()
    # Linux specific settings
    set(EXTRA_LIBS
        GL
        X11
        pthread
        dl
    )
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# Find GLFW package specifically
find_package(glfw3 3.3 REQUIRED)

# Find other required libraries
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GLEW REQUIRED glew)
    pkg_check_modules(GLM REQUIRED glm)
    pkg_check_modules(FFTW REQUIRED fftw3f)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    pkg_check_modules(SNDFILE REQUIRED sndfile)
else()
    find_package(GLEW REQUIRED)
    find_package(GLM REQUIRED)
    
    find_library(FFTW_LIBRARIES NAMES fftw3f)
    find_library(PORTAUDIO_LIBRARIES NAMES portaudio)
    find_library(SNDFILE_LIBRARIES NAMES sndfile)
    
    find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h)
    find_path(PORTAUDIO_INCLUDE_DIRS NAMES portaudio.h)
    find_path(SNDFILE_INCLUDE_DIRS NAMES sndfile.h)
endif()

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

# Source files
set(SOURCES
    src/main.cpp
    src/audio_processor.cpp
    src/visualizer.cpp
    ${IMGUI_SOURCES}
)

# Add platform-specific source files
if(APPLE)
    list(APPEND SOURCES src/file_dialog_mac.mm)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${FFTW_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    GLEW
    ${GLM_LIBRARIES}
    ${FFTW_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${EXTRA_LIBS}
)

# Set Objective-C++ compilation for macOS file dialog
if(APPLE)
    set_source_files_properties(src/file_dialog_mac.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

# Copy shader files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})