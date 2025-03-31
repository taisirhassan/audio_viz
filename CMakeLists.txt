cmake_minimum_required(VERSION 3.12)
project(AudioVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    
    # Set Homebrew prefix for M1/Intel Macs
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew" CACHE PATH "Homebrew installation prefix")
    else()
        set(HOMEBREW_PREFIX "/usr/local" CACHE PATH "Homebrew installation prefix")
    endif()
    
    # Add Homebrew paths
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
    include_directories(SYSTEM ${HOMEBREW_PREFIX}/include)
    link_directories(${HOMEBREW_PREFIX}/lib)
    
    set(EXTRA_LIBS
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreServices"
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
    )
elseif(WIN32)
    set(EXTRA_LIBS opengl32)
else()
    set(EXTRA_LIBS
        GL
        X11
        pthread
        dl
    )
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# Find GLEW
if(APPLE)
    # On macOS, use pkg-config for GLEW
    pkg_check_modules(GLEW REQUIRED glew)
    
    # Add GLFW framework path
    find_library(GLFW_LIBRARY NAMES glfw glfw3 PATHS ${HOMEBREW_PREFIX}/lib)
    if(NOT GLFW_LIBRARY)
        message(FATAL_ERROR "GLFW library not found")
    endif()
else()
    find_package(GLEW REQUIRED)
    find_package(GLFW3 REQUIRED)
endif()

# Find other libraries
pkg_check_modules(GLM REQUIRED glm)
pkg_check_modules(FFTW REQUIRED fftw3f)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(SNDFILE REQUIRED sndfile)

# Verify ImGui directory exists
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/extern/imgui")
if(NOT EXISTS "${IMGUI_DIR}")
    message(FATAL_ERROR "ImGui directory not found at ${IMGUI_DIR}. Please clone ImGui repository.")
endif()

# Add ImGui source files
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

# Source files
set(SOURCES
    src/main.cpp
    src/audio_processor.cpp
    src/visualizer.cpp
    src/ui_manager.cpp
    src/Application.cpp
    src/BarGraphVisualization.cpp
    src/CircularVisualization.cpp
    src/WaveVisualization.cpp
)

# Add ImGui sources to the project
list(APPEND SOURCES ${IMGUI_SOURCES})

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
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${FFTW_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLM_LIBRARIES}
    ${FFTW_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${EXTRA_LIBS}
)

# Set Objective-C++ compilation for macOS file dialog
if(APPLE)
    set_source_files_properties(
        src/file_dialog_mac.mm 
        PROPERTIES 
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
endif()

# Copy shader files to build directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
)

# Enable warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Output configuration messages
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Using Homebrew prefix: ${HOMEBREW_PREFIX}")
message(STATUS "GLFW libraries: ${GLFW_LIBRARIES}")
message(STATUS "GLEW include dirs: ${GLEW_INCLUDE_DIRS}")
message(STATUS "ImGui directory: ${IMGUI_DIR}")
foreach(lib ${EXTRA_LIBS})
    message(STATUS "Extra library: ${lib}")
endforeach()