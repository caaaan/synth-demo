# JUCE Audio Synthesizer Demo

A simple audio synthesizer built with JUCE framework, featuring basic waveform generation and ADSR envelope control.

## Features

- Four basic waveform types:
  - Sine wave
  - Square wave
  - Sawtooth wave
  - Triangle wave
- ADSR envelope control:
  - Attack
  - Decay
  - Sustain
  - Release
- Real-time parameter control
- MIDI input support

## Requirements

- JUCE Framework
- C++17 or later
- CMake 3.15 or later
- A C++ compiler that supports C++17

## Building the Project

1. Clone the repository:
```bash
git clone https://github.com/caaaan/synth-demo.git
cd synth-demo
```

2. Open the project in your IDE:
   - Open `AudioSynthesiserDemo.jucer` in Projucer
   - Or use CMake to generate build files

3. Build the project:
   - In Projucer: Click "Save Project and Open in IDE"
   - With CMake: Follow standard CMake build process

## Usage

1. Launch the application
2. Select a waveform type
3. Adjust ADSR parameters to shape your sound
4. Play notes using MIDI input or virtual keyboard

## Project Structure

- `Source/` - Contains the main source code
- `JuceLibraryCode/` - JUCE-generated code
- `Builds/` - Build configurations for different platforms

## License

This project is open source and available under the MIT License.

## Author

Created by [Your Name] 