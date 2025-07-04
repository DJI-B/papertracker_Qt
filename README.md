# PaperTracker

<div align="center">

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
[![Status: Beta](https://img.shields.io/badge/Status-Beta-orange.svg)]()
[![Qt6](https://img.shields.io/badge/Qt-6.8-green.svg)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()

</div>

[English](README.md) | [中文](README_zh.md)

## Overview

PaperTracker is a high-performance facial expression and eye tracking application designed for VR and real-time applications. Built with advanced deep learning algorithms and optimized C++ implementation, it provides low-latency, high-precision facial expression recognition, eye tracking, and OSC data transmission.

### Core Features

- **🎯 Dual Tracking**: Integrates facial expression and eye tracking for complete facial capture solutions
- **⚡ High-Performance Processing**: C++ optimized implementation with significantly reduced CPU usage for real-time response
- **🔌 Multi-Device Support**: Compatible with ESP32 cameras and local camera devices
- **📡 Standardized Output**: Transmits blendshape data via OSC protocol, compatible with VRChat, Resonite, and other platforms
- **🛠️ Developer-Friendly**: Complete CMake build system with MSVC compiler support

## Setup Guide

For detailed ESP32 device setup: [ESP32 Device Configuration Guide](https://fcnk6r4c64fa.feishu.cn/wiki/B4pNwz2avi4kNqkVNw9cjRRXnth?from=from_copylink)

## Main Features

### Tracking Capabilities
- 👁️ **Eye Tracking**: Based on ETVR technology, supports blinking, eye movement, pupil tracking, and other fine movements
- 👄 **Facial Expression Tracking**: Based on Project Babble technology, recognizes speech, smiling, surprise, and other facial expressions
- 🎭 **Expression Blending**: Outputs standard blendshape data supporting composite expressions and eye movements

### Hardware Interface
- 📷 **ESP32 Camera**: Wireless connection with firmware update and WiFi configuration support
- 💻 **Local Camera**: Supports USB cameras and built-in cameras
- 🔧 **Firmware Management**: Integrated ESP32 firmware flashing and update tools
- 👀 **Eye Tracking Device**: Supports ETVR-compatible eye tracking hardware

### Data Output
- 📊 **OSC Protocol**: Real-time transmission of blendshape and eye tracking data
- 📈 **Performance Monitoring**: Real-time display of framerate, latency, and other performance metrics
- 📝 **Logging System**: Detailed runtime status and error information logging

## System Requirements

### Hardware Requirements
- **Operating System**: Windows 10/11 (x64)
- **Processor**: Modern CPU with AVX2 instruction set support
- **Memory**: At least 4GB RAM (8GB recommended)
- **Camera Devices**:
  - **Facial Tracking**: ESP32-S3 device + OV2640 camera (requires [face_tracker firmware](https://github.com/paper-tei/face_tracker)) or USB/built-in camera
  - **Eye Tracking**: ETVR-compatible eye tracking device (optional)

### Software Dependencies
- Visual C++ Redistributable 2022
- .NET Framework 4.8 (for firmware update tools)

## Quick Start

### Installation Steps

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

## Development and Building

### Development Environment Setup

#### Required Tools
```bash
# Windows Build Toolchain
- Visual Studio 2022 Community/Professional
- CMake 3.30+
- Qt 6.8.2 MSVC version
```

#### Third-Party Dependencies
```bash
# Automatically managed dependencies
- OpenCV 4.x
- ONNX Runtime 1.20.1
- libcurl
- OSCPack
```

### Build Steps

```bash
# 1. Clone project
git clone https://github.com/yourusername/PaperTracker.git
cd PaperTracker

# 2. Configure CMake
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. Build project
cmake --build . --config Release

# 4. Install dependency files
cmake --install .
```

### CMakeLists.txt Configuration

The project uses modular CMake configuration with the following main components:

```cmake
# Core Components
- utilities: Logging, update utilities library
- algorithm: Inference algorithm library  
- transfer: Data transmission library
- ui: User interface library
- oscpack: OSC protocol library
```

### Qt6 Module Dependencies

```cmake
find_package(Qt6 COMPONENTS 
    Core Gui Widgets 
    Network WebSockets SerialPort 
    REQUIRED
)
```

## License Information

### Main Project License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License** (CC BY-NC-SA 4.0).

#### License Features

**✅ You are free to**
- **Share**: Copy and redistribute the material in any medium or format
- **Adapt**: Remix, transform, and build upon the material
- **Personal Use**: Use for personal learning, research, and non-commercial purposes

**📋 Under the following terms**
- **Attribution**: You must give appropriate credit, provide a link to the license, and indicate if changes were made
- **NonCommercial**: You may not use the material for commercial purposes
- **ShareAlike**: If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original

**❌ Restrictions**
- **Prohibited Commercial Use**: May not be used for commercial purposes, including but not limited to:
  - Selling this software or products based on this software
  - Integrating into commercial hardware products
  - Providing paid services based on this software
  - Using in commercial environments (except for research purposes)

### Third-Party Component Licenses

#### Project Babble 2.0.7 Components
```
Source: Project Babble v2.0.7
License: Babble Software Distribution License 1.0
Usage Scope: Facial expression recognition models
Usage Restrictions: Non-commercial use (consistent with this project's license)
Project URL: https://github.com/Project-Babble/ProjectBabble
```

#### EyeTrackVR (ETVR) Components
```
Source: EyeTrackVR early open source version
License: MIT License
Usage Scope: Eye tracking algorithms and related code
Usage Timeline: Based on open source version before license updates
Project URL: https://github.com/EyeTrackVR
```

#### Other Dependencies
```
- OpenCV: BSD 3-Clause License
- ONNX Runtime: MIT License  
- Qt6: GPL/LGPL dual licensing
- libcurl: MIT License
- OSCPack: Custom open source license
```

For complete third-party component information, see [THIRD-PARTY.txt](THIRD-PARTY.txt).

## Commercial Use Information

### Non-Commercial License Restrictions

This project uses CC BY-NC-SA 4.0 license, which **strictly prohibits any form of commercial use**, including but not limited to:

- ❌ **Software Sales**: May not sell this software or products containing this software
- ❌ **Hardware Integration**: May not integrate this software into commercial hardware products
- ❌ **Commercial Services**: May not provide paid services based on this software
- ❌ **Enterprise Use**: May not use in commercial environments (except for research purposes)

### Applicable Use Cases

**✅ Permitted Uses**
- Personal learning and entertainment
- Academic research and education
- Non-profit organization projects
- Open source project development
- VR enthusiast personal projects

### Commercial Licensing

If you need to use this project for commercial purposes, please contact us for special commercial licensing through:
- GitHub Issues: Explain your commercial use requirements
- Project Homepage: Check contact information

## Contributing Guidelines

### Development Participation

We welcome all forms of non-commercial contributions:

1. **Issue Reporting**: Report bugs or suggest new features via [GitHub Issues](../../issues)
2. **Code Contributions**: Submit Pull Requests to improve code quality
3. **Documentation Improvement**: Help improve project documentation and usage guides
4. **Testing Support**: Test in different environments and provide compatibility feedback

### Contributor License Agreement

By contributing code to this project, you agree that:
- Your contributions will be released under CC BY-NC-SA 4.0 license
- You have legal rights to the contributed content
- Your contributions will not violate any third-party rights

### Code Standards

#### C++ Coding Standards
```cpp
// 1. Use C++20 standard features
// 2. Follow RAII principles
// 3. Prefer smart pointers
// 4. Unified naming convention (snake_case)
```

#### Submission Requirements
- Ensure code compiles under MSVC 2022
- Include appropriate unit tests
- Follow existing code style
- Provide clear commit messages

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
```

## Contact Information

- **Issue Reporting**: [GitHub Issues](../../issues)
- **Feature Suggestions**: [GitHub Discussions](../../discussions)
- **Technical Communication**: Please use Issues
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