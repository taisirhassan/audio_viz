# Audio Visualizer

<img width="1198" alt="Screenshot 2024-09-07 at 10 11 25 PM" src="https://github.com/user-attachments/assets/1fef96cf-c949-4931-982e-22b83508bd92">


An interactive real-time audio visualizer built with C++ and OpenGL. This application provides dynamic visualizations of audio input, supporting both live audio and file playback.

## Overview

This project combines audio processing and graphics rendering to create an engaging visual representation of audio data. It offers multiple visualization styles and interactive controls for a customizable experience.

### Key Features

- Real-time audio processing and visualization
- Multiple visualization styles: Bar Graph, Circular, and Wave
- Support for both live audio input and audio file playback
- User-friendly interface with adjustable parameters

### Technologies Used

- $\textbf{C++}$ for core application logic
- $\textbf{OpenGL}$ for graphics rendering
- $\textbf{GLFW}$ for window management and OpenGL context creation
- $\textbf{Dear ImGui}$ for the graphical user interface
- $\textbf{PortAudio}$ for real-time audio input/output
- $\textbf{FFTW}$ for Fast Fourier Transform calculations
- $\textbf{libsndfile}$ for audio file reading

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

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/audio-visualizer.git
   cd audio-visualizer
   ```

2. Create a build directory and navigate into it:
   ```bash
   mkdir build && cd build
   ```

3. Generate the build files with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   make
   ```

5. Run the application:
   ```bash
   ./AudioVisualizer
   ```

## Usage

Once the application is running:

1. Use the radio buttons to switch between visualization styles.
2. Adjust the "Rotation Speed" and "Zoom Level" sliders to customize the visualization.
3. Click "Toggle Audio Source" to switch between live audio input and file playback.
4. Use the "Load Audio File" button to select and load an audio file for playback.

## Contributing

Contributions to improve the Audio Visualizer are welcome. Please feel free to submit pull requests or open issues to discuss potential changes or enhancements.
