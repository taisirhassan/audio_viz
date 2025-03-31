# Audio Visualizer

<img width="1280" alt="image" src="https://github.com/user-attachments/assets/46efb426-dd9b-42b8-b13b-982ce3b2413d" />


An interactive real-time audio visualizer built with C++ and OpenGL. This application provides dynamic visualizations of audio input, supporting both live audio and file playback.

## Overview

This project combines audio processing and graphics rendering to create an engaging visual representation of audio data. It offers multiple visualization styles and interactive controls for a customizable experience.

### Key Features

- Real-time audio processing and visualization
- Multiple visualization styles: Bar Graph, Circular, and Wave
- Support for both live audio input and audio file playback
- User-friendly interface with adjustable parameters
- Native file dialogs for file selection
- Modular architecture for maintainability and extensibility

### Technologies Used

- **C++** for core application logic
- **OpenGL** for graphics rendering
- **GLFW** for window management and OpenGL context creation
- **Dear ImGui** for the graphical user interface
- **PortAudio** for real-time audio input/output
- **FFTW** for Fast Fourier Transform calculations
- **libsndfile** for audio file reading
- **Native File Dialog Extended** for cross-platform file dialogs

## Architecture

The application is designed with a modular architecture that separates concerns:

- **Application**: Main application logic, window management, and event handling
- **AudioProcessor**: Handles audio input/output, FFT, and audio file playback
- **Visualizer**: Manages rendering different visualization styles
- **UIManager**: Handles UI rendering and user input
- **VisualizationStyles**: Pluggable visualization implementations (Bar Graph, Circular, Wave)

## Setup and Installation

### Prerequisites

Ensure you have the following installed on your system:
- CMake (version 3.12 or higher)
- C++ compiler with C++17 support
- OpenGL
- GLFW
- GLEW
- PortAudio
- FFTW
- libsndfile

On macOS, you can install most dependencies using Homebrew:

```bash
brew install cmake glfw glew fftw portaudio libsndfile
```

### Additional Dependencies

The project uses some external dependencies that are included as Git submodules:

1. **Dear ImGui**: UI library
2. **Native File Dialog Extended**: Cross-platform file dialog library

Clone these submodules after cloning the main repository:

```bash
git submodule update --init --recursive
```

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/audio-visualizer.git
   cd audio-visualizer
   ```

2. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

3. Create a build directory and navigate into it:
   ```bash
   mkdir build && cd build
   ```

4. Generate the build files with CMake:
   ```bash
   cmake ..
   ```

5. Build the project:
   ```bash
   make
   ```

6. Run the application:
   ```bash
   ./AudioVisualizer
   ```

## Usage

Once the application is running:

1. Use the dropdown menu to select an audio input device
2. Choose your preferred visualization style (Bar Graph, Circular, or Wave)
3. Adjust the "Rotation Speed" and "Zoom Level" sliders to customize the visualization
4. Modify the color scheme using the color pickers
5. Open the "Audio Processing" section to adjust smoothing and gain parameters
6. Use the "File Playback" section to open audio files, play/pause, or switch back to microphone input

## Contributing

Contributions to improve the Audio Visualizer are welcome. Please feel free to submit pull requests or open issues to discuss potential changes or enhancements.
