# PaperTracker

<div align="center">

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
[![Status: Beta](https://img.shields.io/badge/Status-Beta-orange.svg)]()
[![Qt6](https://img.shields.io/badge/Qt-6.8.2-green.svg)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()
[![OpenCV](https://img.shields.io/badge/OpenCV-4.11.0-blue.svg)](https://opencv.org/)
[![ONNX Runtime](https://img.shields.io/badge/ONNX%20Runtime-1.22.0-orange.svg)](https://onnxruntime.ai/)
[![State-of-the-art Shitcode](https://img.shields.io/static/v1?label=State-of-the-art&message=Shitcode&color=7B5804)](https://github.com/trekhleb/state-of-the-art-shitcode)

</div>

[English](README.md) | [中文](README_zh.md)

## Overview

PaperTracker is a high-performance facial expression and eye tracking application built with C++20 and Qt6, designed for VR and real-time applications. The project features modern CMake build system integration with advanced deep learning algorithms, providing low-latency, high-precision facial expression recognition, eye tracking, and OSC data transmission capabilities.

### Core Features

- **🎯 Dual Tracking**: Integrated facial expression and eye tracking for complete facial capture solutions
- **⚡ High Performance**: C++20 optimized implementation with CUDA acceleration support for significantly reduced latency
- **🔌 Multi-Device Support**: Compatible with ESP32 cameras, USB cameras, and ETVR eye tracking devices
- **📡 Standardized Output**: OSC protocol transmission of blendshape data, compatible with VRChat, Resonite, and other platforms
- **🛠️ Developer Friendly**: Complete CMake build system with automatic dependency management and MSVC compiler support
- **🌐 Internationalization**: Built-in multi-language support and translation management system

## System Architecture

### Modular Design
The project adopts a modular architecture with the following core components:

```
PaperTracker/
├── utilities/          # Common utilities library (logging, updater, etc.)
├── algorithm/          # AI inference algorithm library (face/eye recognition)
├── transfer/          # Data transmission library (serial, network, OSC)
├── ui/                # User interface library (Qt6 UI components)
├── model/             # AI model files
├── translations/      # Internationalization translation files
├── resources/         # Application resource files
└── 3rdParty/         # Third-party dependency libraries (auto-managed)
```

### Technology Stack
- **Frontend Framework**: Qt6.8.2 (Core, Gui, Widgets, Network, WebSockets, SerialPort)
- **Computer Vision**: OpenCV 4.11.0
- **AI Inference Engine**: ONNX Runtime 1.22.0 (CPU and CUDA GPU acceleration support)
- **Network Communication**: OSCPack, WebSockets, HTTP Server
- **Build System**: CMake 3.30+ with MSVC 2022
- **Hardware Interface**: ESP32 firmware management, serial communication

## System Requirements

### Hardware Requirements
- **Operating System**: Windows 10/11 (x64)
- **Processor**: Modern CPU with AVX2 instruction set support
- **Memory**: At least 4GB RAM (8GB+ recommended)
- **Graphics Card**: Optional NVIDIA GPU (CUDA 11.x/12.x acceleration support)
- **Camera Devices**:
  - **Facial Tracking**: ESP32-S3 device + OV2640 camera or USB camera
  - **Eye Tracking**: ETVR-compatible eye tracking device (optional)

### Software Dependencies
- Visual C++ Redistributable 2022
- .NET Framework 4.8 (firmware update tools)
- Windows SDK (development environment)

## Development Environment Setup

### Required Tools
```bash
# Windows Development Toolchain
- Visual Studio 2022 Community/Professional (including MSVC v143 toolset)
- CMake 3.30 or higher
- Qt 6.8.2 MSVC version
- Git (for pulling third-party dependencies)
```

### Qt6 Configuration
```cmake
# Qt6 modules supported by the project
find_package(Qt6 COMPONENTS 
    Core Gui Widgets           # Basic UI components
    Network WebSockets         # Network communication
    SerialPort                 # Serial communication
    REQUIRED
)
```

### CUDA Support (Optional)
If CUDA is installed on the system, the project will automatically enable GPU acceleration:
```cmake
# CUDA support configuration
find_package(CUDA)
if(CUDA_FOUND)
    add_definitions(-DUSE_CUDA)
    # Automatically configure cuDNN libraries and runtime dependencies
endif()
```

## Build Steps

### 1. Environment Preparation
```bash
# Ensure required tools are installed
- Visual Studio 2022 with C++ workload
- CMake 3.30+
- Qt 6.8.2 (MSVC 2022 64-bit)
```

### 2. Clone Project
```bash
git clone https://github.com/your-org/PaperTracker.git
cd PaperTracker
```

### 3. Configure CMake
```bash
# Create build directory
mkdir build
cd build

# Configure project (Debug mode)
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DQT_INSTALL_PATH="D:/QtNew/6.8.2/msvc2022_64"

# Or configure Release mode
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQT_INSTALL_PATH="D:/QtNew/6.8.2/msvc2022_64"
```

### 4. Build Project
```bash
# Build Debug version
cmake --build . --config Debug

# Build Release version
cmake --build . --config Release
```

### 5. Install Dependencies
```bash
# Copy runtime dependencies to build directory
cmake --install .
```

## Dependency Management

### Automatic Dependency Download
The project uses CMake's FetchContent functionality to automatically manage third-party dependencies:

```cmake
# OpenCV 4.11.0 - Computer Vision Library
FetchContent_Declare(prebuilt_opencv 
    URL https://github.com/opencv/opencv/releases/download/4.11.0/opencv-4.11.0-windows.exe)

# ONNX Runtime 1.22.0 - AI Inference Engine
FetchContent_Declare(onnxruntime 
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-win-x64-1.22.0.zip)

# OSCPack - OSC Protocol Library
ExternalProject_Add(oscpack 
    GIT_REPOSITORY https://github.com/RossBencina/oscpack.git)

# ESPTool - ESP32 Firmware Flashing Tool
FetchContent_Declare(esptool 
    URL https://github.com/espressif/esptool/releases/download/v5.0.0/esptool-v5.0.0-windows-amd64.zip)
```

### Directory Structure
```
build/
├── 3rdParty/          # Auto-downloaded third-party libraries
│   ├── opencv/        # OpenCV 4.11.0
│   ├── onnxruntime/   # ONNX Runtime 1.22.0
│   ├── oscpack/       # OSCPack library
│   └── esptools/      # ESP32 tools
├── Debug/             # Debug build output
├── Release/           # Release build output
└── model/             # AI model files
```

## Configuration Details

### CMake Configuration Options
```cmake
# Configurable CMake options
set(QT_INSTALL_PATH "D:/QtNew/6.8.2/msvc2022_64" CACHE PATH "Qt installation path")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type: Debug or Release")
set(CMAKE_CXX_STANDARD 20)                         # C++20 standard
set(CMAKE_AUTOMOC ON)                              # Qt MOC auto-processing
set(CMAKE_AUTORCC ON)                              # Qt resource file auto-processing
set(CMAKE_AUTOUIC ON)                              # Qt UI file auto-processing
```

### MSVC Compiler Optimization
```cmake
if (MSVC)
    # Preprocessor optimization
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")
    # UTF-8 encoding support
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    # Warning level settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    # Runtime library configuration
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    endif()
endif()
```

## Quick Start

### Install Pre-compiled Version
1. **Download Application**
   ```
   Download the latest version from [Releases](../../releases) page
   ```

2. **Install Runtime Environment**
   ```
   Install Visual C++ Redistributable 2022
   ```

3. **Extract and Run**
   ```
   Extract the package to any directory
   Run PaperTracker.exe
   ```

### Initial Configuration

#### Facial Tracking Setup
1. Connect ESP32 device to the same WiFi network or select local camera
2. Configure device connection in the application
3. Adjust camera parameters and Region of Interest (ROI)
4. Start facial expression tracking

#### Eye Tracking Setup (Optional)
1. Connect ETVR-compatible eye tracking device
2. Configure serial port connection parameters
3. Perform eye calibration process
4. Start eye tracking

### VR Application Integration

#### VRChat Configuration
```
OSC Port: 9000
IP Address: 127.0.0.1
Enable facial tracking and eye tracking components
```

#### Resonite Configuration
```
Use OSC Receiver component
Listen Port: 9000
Data Format: VRC-compatible blendshape
```

## Troubleshooting

### Common Build Issues

#### Qt6 Path Configuration
```cmake
# If CMake cannot find Qt6, set the correct Qt installation path
set(QT_INSTALL_PATH "Your-Qt-Installation-Path" CACHE PATH "Qt installation directory path")
```

#### OpenCV Download Failure
```bash
# Manually download OpenCV and place in the specified directory
mkdir 3rdParty/opencv
# Download and extract OpenCV to this directory
```

#### ONNX Runtime Configuration Issues
```cmake
# Check ONNX Runtime path configuration
set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime)
```

### Runtime Issues

#### Missing DLL Errors
The project automatically copies required DLL files to the build directory:
```cmake
# Qt6 DLL auto-copy
# OpenCV DLL auto-copy  
# ONNX Runtime DLL auto-copy
# CUDA DLL auto-copy (if enabled)
```

#### Missing Resource Files
```cmake
# Resource files auto-installation
install(DIRECTORY ${CMAKE_SOURCE_DIR}/model/ DESTINATION ${CMAKE_BINARY_DIR}/model)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/translations/ DESTINATION ${CMAKE_BINARY_DIR}/translations)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
```

## Performance Optimization

### System Tuning Recommendations
```
1. CPU Optimization
   - Enable AVX2 instruction set support
   - Set high-performance power plan
   - Close unnecessary background programs

2. Memory Optimization  
   - Ensure sufficient available RAM
   - Consider using dedicated GPU for inference acceleration

3. Network Optimization (ESP32 mode)
   - Reduce network latency and packet loss
   - Use 5GHz WiFi band
```

### CUDA Acceleration Configuration
```cmake
# Automatically detect and configure CUDA support
if(CUDA_FOUND)
    # CUDA architecture configuration
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
    # Link CUDA runtime library
    target_link_libraries(PaperTracker PRIVATE CUDA::cudart)
    # Link ONNX Runtime CUDA providers
    target_link_libraries(PaperTracker PRIVATE onnxruntime_providers_cuda)
endif()
```

## Development Guide

### Code Standards
```cpp
// 1. Use C++20 modern features
// 2. Follow Qt coding conventions
// 3. Use smart pointers for memory management
// 4. Unified naming convention (snake_case)
```

### Submission Requirements
- Ensure code compiles under MSVC 2022
- Include appropriate unit tests
- Follow existing code style
- Provide clear commit messages

## Configuration Guides

For detailed configuration steps, please refer to:
- [ESP32 Device Configuration Guide](https://fcnk6r4c64fa.feishu.cn/wiki/B4pNwz2avi4kNqkVNw9cjRRXnth?from=from_copylink)
- [Facial Tracking Manual](https://fcnk6r4c64fa.feishu.cn/wiki/LZdrwWWozi7zffkLt5pc81WanAd)
- [Eye Tracking Manual](https://fcnk6r4c64fa.feishu.cn/wiki/Dg4qwI3mDiJ3fHk5iZtc2z6Rn47)

## License Information

### Main Project License
This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License** (CC BY-NC-SA 4.0).

### Third-Party Components
- Qt6: LGPL v3 License
- OpenCV: Apache 2.0 License
- ONNX Runtime: MIT License
- OSCPack: Custom Open Source License

## Contact Information

- **Issue Reporting**: [GitHub Issues](../../issues)
- **Feature Suggestions**: [GitHub Discussions](../../discussions)
- **Technical Communication**: Please use Issues for discussions
- **Commercial Licensing**: Please contact via project homepage

## Acknowledgments

### Special Contributors
- **[JellyfishKnight (Liu Han)](https://github.com/JellyfishKnight)**: Provided important technical support and code contributions to the project

### Open Source Projects and Teams
- **Project Babble Team**: Providing excellent facial tracking algorithms and models
- **EyeTrackVR Team**: Providing open source eye tracking technology
- **Qt Community**: Providing powerful cross-platform GUI framework
- **OpenCV Team**: Providing computer vision foundation library

### Community Support
- **All Contributors**: Thanks to every developer who contributed to the project

---

<div align="center">

**This project is for non-commercial use only | Released under CC BY-NC-SA 4.0 License**

**If this project helps you, please consider giving us a Star ⭐**

[⬆️ Back to Top](#papertracker)

</div>
