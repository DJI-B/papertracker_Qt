# PaperTracker

<div align="center">

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
[![Status: Beta](https://img.shields.io/badge/Status-Beta-orange.svg)]()
[![Qt6](https://img.shields.io/badge/Qt-6.8.2-green.svg)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()
[![OpenCV](https://img.shields.io/badge/OpenCV-4.11.0-blue.svg)](https://opencv.org/)
[![ONNX Runtime](https://img.shields.io/badge/ONNX%20Runtime-1.22.0-orange.svg)](https://onnxruntime.ai/)

</div>

[English](README.md) | [中文](README_zh.md)

## 项目简介

PaperTracker 是一款基于C++20和Qt6开发的高性能面部表情和眼部追踪应用程序，专为VR和实时应用设计。项目采用现代化的CMake构建系统，集成了先进的深度学习算法，提供低延迟、高精度的面部表情识别、眼部追踪与OSC数据传输功能。

### 核心特性

- **🎯 双重追踪**：集成面部表情和眼部追踪，提供完整的面部捕捉解决方案
- **⚡ 高性能处理**：C++20优化实现，支持CUDA加速，显著降低延迟
- **🔌 多设备支持**：兼容ESP32摄像头、USB摄像头和ETVR眼追设备
- **📡 标准化输出**：通过OSC协议传输blendshape数据，兼容VRChat、Resonite等平台
- **🛠️ 开发者友好**：完整的CMake构建系统，自动依赖管理，支持MSVC编译器
- **🌐 国际化支持**：内置多语言支持和翻译管理系统

## 系统架构

### 模块化设计
项目采用模块化架构，包含以下核心组件：

```
PaperTracker/
├── utilities/          # 通用工具库（日志、更新器等）
├── algorithm/          # AI推理算法库（面部/眼部识别）
├── transfer/          # 数据传输库（串口、网络、OSC）
├── ui/                # 用户界面库（Qt6界面组件）
├── model/             # AI模型文件
├── translations/      # 国际化翻译文件
├── resources/         # 应用资源文件
└── 3rdParty/         # 第三方依赖库（自动管理）
```

### 技术栈
- **前端框架**：Qt6.8.2 (Core, Gui, Widgets, Network, WebSockets, SerialPort)
- **计算机视觉**：OpenCV 4.11.0
- **AI推理引擎**：ONNX Runtime 1.22.0 (支持CPU和CUDA GPU加速)
- **网络通信**：OSCPack、WebSockets、HTTP服务器
- **构建系统**：CMake 3.30+ with MSVC 2022
- **硬件接口**：ESP32固件管理、串口通信

## 系统要求

### 硬件要求
- **操作系统**：Windows 10/11 (x64)
- **处理器**：支持AVX2指令集的现代CPU
- **内存**：至少4GB RAM（推荐8GB以上）
- **显卡**：可选NVIDIA GPU（支持CUDA 11.x/12.x加速）
- **摄像头设备**：
  - **面部追踪**：ESP32-S3设备 + OV2640摄像头或USB摄像头
  - **眼部追踪**：ETVR兼容的眼追设备（可选）

### 软件依赖
- Visual C++ Redistributable 2022
- .NET Framework 4.8（固件更新工具）
- Windows SDK（开发环境）

## 开发环境配置

### 必需工具
```bash
# Windows开发工具链
- Visual Studio 2022 Community/Professional (包含MSVC v143工具集)
- CMake 3.30或更高版本
- Qt 6.8.2 MSVC版本
- Git (用于拉取第三方依赖)
```

### Qt6配置
```cmake
# 项目支持的Qt6模块
find_package(Qt6 COMPONENTS 
    Core Gui Widgets           # 基础UI组件
    Network WebSockets         # 网络通信
    SerialPort                 # 串口通信
    REQUIRED
)
```

### CUDA支持（可选）
如果系统安装了CUDA，项目将自动启用GPU加速：
```cmake
# CUDA支持配置
find_package(CUDA)
if(CUDA_FOUND)
    add_definitions(-DUSE_CUDA)
    # 自动配置cuDNN库和运行时依赖
endif()
```

## 构建步骤

### 1. 环境准备
```bash
# 确保已安装必需工具
- Visual Studio 2022 with C++ workload
- CMake 3.30+
- Qt 6.8.2 (MSVC 2022 64-bit)
```

### 2. 克隆项目
```bash
git clone https://github.com/your-org/PaperTracker.git
cd PaperTracker
```

### 3. 配置CMake
```bash
# 创建构建目录
mkdir build
cd build

# 配置项目（Debug模式）
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DQT_INSTALL_PATH="D:/QtNew/6.8.2/msvc2022_64"

# 或配置Release模式
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DQT_INSTALL_PATH="D:/QtNew/6.8.2/msvc2022_64"
```

### 4. 构建项目
```bash
# 构建Debug版本
cmake --build . --config Debug

# 构建Release版本
cmake --build . --config Release
```

### 5. 安装依赖文件
```bash
# 复制运行时依赖到构建目录
cmake --install .
```

## 依赖管理

### 自动依赖下载
项目使用CMake的FetchContent功能自动管理第三方依赖：

```cmake
# OpenCV 4.11.0 - 计算机视觉库
FetchContent_Declare(prebuilt_opencv 
    URL https://github.com/opencv/opencv/releases/download/4.11.0/opencv-4.11.0-windows.exe)

# ONNX Runtime 1.22.0 - AI推理引擎
FetchContent_Declare(onnxruntime 
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-win-x64-1.22.0.zip)

# OSCPack - OSC协议库
ExternalProject_Add(oscpack 
    GIT_REPOSITORY https://github.com/RossBencina/oscpack.git)

# ESPTool - ESP32固件烧录工具
FetchContent_Declare(esptool 
    URL https://github.com/espressif/esptool/releases/download/v5.0.0/esptool-v5.0.0-windows-amd64.zip)
```

### 目录结构
```
build/
├── 3rdParty/          # 自动下载的第三方库
│   ├── opencv/        # OpenCV 4.11.0
│   ├── onnxruntime/   # ONNX Runtime 1.22.0
│   ├── oscpack/       # OSCPack库
│   └── esptools/      # ESP32工具
├── Debug/             # Debug构建输出
├── Release/           # Release构建输出
└── model/             # AI模型文件
```

## 配置说明

### CMake配置选项
```cmake
# 可配置的CMake选项
set(QT_INSTALL_PATH "D:/QtNew/6.8.2/msvc2022_64" CACHE PATH "Qt安装路径")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "构建类型：Debug或Release")
set(CMAKE_CXX_STANDARD 20)                         # C++20标准
set(CMAKE_AUTOMOC ON)                              # Qt MOC自动处理
set(CMAKE_AUTORCC ON)                              # Qt资源文件自动处理
set(CMAKE_AUTOUIC ON)                              # Qt UI文件自动处理
```

### MSVC编译器优化
```cmake
if (MSVC)
    # 预处理器优化
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")
    # UTF-8编码支持
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    # 警告级别设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    # 运行时库配置
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    endif()
endif()
```

## 快速开始

### 安装预编译版本
1. **下载应用程序**
   ```
   从 [Releases](../../releases) 页面下载最新版本
   ```

2. **安装运行环境**
   ```
   安装 Visual C++ Redistributable 2022
   ```

3. **解压并运行**
   ```
   解压安装包到任意目录
   运行 PaperTracker.exe
   ```

### 首次配置

#### 面部追踪设置
1. 连接ESP32设备到同一WiFi网络或选择本地摄像头
2. 在应用中配置设备连接
3. 调整摄像头参数和感兴趣区域(ROI)
4. 开始面部表情追踪

#### 眼部追踪设置（可选）
1. 连接ETVR兼容的眼追设备
2. 配置串口连接参数
3. 执行眼部校准流程
4. 开始眼部追踪

### VR应用集成

#### VRChat配置
```
OSC端口：9000
IP地址：127.0.0.1
启用面部追踪和眼部追踪组件
```

#### Resonite配置
```
使用OSC接收器组件
监听端口：9000
数据格式：VRC兼容blendshape
```

## 故障排除

### 常见构建问题

#### Qt6路径配置
```cmake
# 如果CMake找不到Qt6，请设置正确的Qt安装路径
set(QT_INSTALL_PATH "你的Qt安装路径" CACHE PATH "Qt安装目录路径")
```

#### OpenCV下载失败
```bash
# 手动下载OpenCV并放置到指定目录
mkdir 3rdParty/opencv
# 下载并解压OpenCV到该目录
```

#### ONNX Runtime配置问题
```cmake
# 检查ONNX Runtime路径配置
set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime)
```

### 运行时问题

#### DLL缺失错误
项目会自动复制所需的DLL文件到构建目录：
```cmake
# Qt6 DLL自动复制
# OpenCV DLL自动复制  
# ONNX Runtime DLL自动复制
# CUDA DLL自动复制（如果启用）
```

#### 资源文件缺失
```cmake
# 资源文件自动安装
install(DIRECTORY ${CMAKE_SOURCE_DIR}/model/ DESTINATION ${CMAKE_BINARY_DIR}/model)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/translations/ DESTINATION ${CMAKE_BINARY_DIR}/translations)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
```

## 性能优化

### 系统调优建议
```
1. CPU优化
   - 启用AVX2指令集支持
   - 设置高性能电源计划
   - 关闭不必要的后台程序

2. 内存优化  
   - 确保足够的可用RAM
   - 考虑使用专用GPU进行推理加速

3. 网络优化（ESP32模式）
   - 降低网络延迟和丢包率
   - 使用5GHz WiFi频段
```

### CUDA加速配置
```cmake
# 自动检测并配置CUDA支持
if(CUDA_FOUND)
    # CUDA架构配置
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
    # 链接CUDA运行时库
    target_link_libraries(PaperTracker PRIVATE CUDA::cudart)
    # 链接ONNX Runtime CUDA提供程序
    target_link_libraries(PaperTracker PRIVATE onnxruntime_providers_cuda)
endif()
```

## 开发指南

### 代码规范
```cpp
// 1. 使用C++20现代特性
// 2. 遵循Qt编码规范
// 3. 使用智能指针管理内存
// 4. 统一命名约定（snake_case）
```

### 提交要求
- 确保代码在MSVC 2022下编译通过
- 包含适当的单元测试
- 遵循现有代码风格
- 提供清晰的提交消息

## 版权信息

### 主项目许可证
本项目采用 **知识共享署名-非商业性使用-相同方式共享 4.0 国际许可协议** (CC BY-NC-SA 4.0) 进行许可。

### 第三方组件
- Qt6：LGPL v3许可证
- OpenCV：Apache 2.0许可证
- ONNX Runtime：MIT许可证
- OSCPack：自定义开源许可证

## 联系信息

- **问题报告**：[GitHub Issues](../../issues)
- **功能建议**：[GitHub Discussions](../../discussions)
- **技术交流**：请使用Issues进行讨论
- **商业授权**：请通过项目主页联系

## 致谢

### 特别贡献者
- **[JellyfishKnight (Liu Han)](https://github.com/JellyfishKnight)**：为项目提供了重要的技术支持和代码贡献

### 开源项目和团队
- **Project Babble团队**：提供优秀的面部追踪算法和模型
- **EyeTrackVR团队**：提供开源眼部追踪技术
- **Qt社区**：提供强大的跨平台GUI框架
- **OpenCV团队**：提供计算机视觉基础库

### 社区支持
- **所有贡献者**：感谢每一位为项目做出贡献的开发者

---

<div align="center">

**本项目仅供非商业用途使用 | 基于 CC BY-NC-SA 4.0 许可证发布**

**如果这个项目对您有帮助，请考虑给我们一个 Star ⭐**

[⬆️ 返回顶部](#papertracker)

</div>
