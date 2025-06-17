# PaperTracker

<div align="center">

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
[![Status: Beta](https://img.shields.io/badge/Status-Beta-orange.svg)]()
[![Qt6](https://img.shields.io/badge/Qt-6.8-green.svg)](https://www.qt.io/)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()

</div>

[English](README.md) | [中文](README_zh.md)

## 项目简介

PaperTracker 是一款高性能的面部表情和眼部追踪应用程序，专为VR和实时应用设计。使用先进的深度学习算法和优化的C++实现，提供低延迟、高精度的面部表情识别、眼部追踪与OSC数据传输功能。

### 核心特性

- **🎯 双重追踪**：集成面部表情和眼部追踪，提供完整的面部捕捉解决方案
- **⚡ 高性能处理**：C++优化实现，显著降低CPU占用，提供实时响应
- **🔌 多设备支持**：兼容ESP32摄像头和本地摄像头设备
- **📡 标准化输出**：通过OSC协议传输blendshape数据，兼容VRChat、Resonite等平台
- **🛠️ 开发者友好**：完整的CMake构建系统，支持MSVC编译器

## 配网指南

详细配网步骤请参考：[ESP32设备配网指南](https://fcnk6r4c64fa.feishu.cn/wiki/B4pNwz2avi4kNqkVNw9cjRRXnth?from=from_copylink)

## 主要功能

### 追踪功能
- 👁️ **眼部追踪**：基于ETVR技术，支持眨眼、眼球转动、瞳孔追踪等精细动作
- 👄 **面部表情追踪**：基于Project Babble技术，识别说话、微笑、惊讶等表情动作
- 🎭 **表情融合**：输出标准blendshape数据，支持复合表情和眼部动作

### 硬件接口
- 📷 **ESP32摄像头**：无线连接，支持固件更新和WiFi配置
- 💻 **本地摄像头**：支持USB摄像头和内置摄像头
- 🔧 **固件管理**：集成ESP32固件烧录和更新工具
- 👀 **眼追设备**：支持ETVR兼容的眼部追踪硬件

### 数据输出
- 📊 **OSC协议**：实时传输blendshape和眼追数据
- 📈 **性能监控**：实时显示帧率、延迟等性能指标
- 📝 **日志系统**：详细的运行状态和错误信息记录

## 系统要求

### 硬件要求
- **操作系统**：Windows 10/11 (x64)
- **处理器**：支持AVX2指令集的现代CPU
- **内存**：至少4GB RAM（推荐8GB）
- **摄像头设备**：
  - **面部追踪**：ESP32-S3设备 + OV2640摄像头（需烧录[face_tracker固件](https://github.com/paper-tei/face_tracker)）或USB/内置摄像头
  - **眼部追踪**：ETVR兼容的眼追设备（可选）

### 软件依赖
- Visual C++ Redistributable 2022
- .NET Framework 4.8（用于固件更新工具）

## 快速开始

### 安装步骤

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

### 与VR应用集成

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

## 开发与构建

### 开发环境配置

#### 必需工具
```bash
# Windows构建工具链
- Visual Studio 2022 Community/Professional
- CMake 3.30+
- Qt 6.8.2 MSVC版本
```

#### 第三方依赖
```bash
# 自动管理的依赖
- OpenCV 4.x
- ONNX Runtime 1.20.1
- libcurl
- OSCPack
```

### 构建步骤

```bash
# 1. 克隆项目
git clone https://github.com/yourusername/PaperTracker.git
cd PaperTracker

# 2. 配置CMake
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. 构建项目
cmake --build . --config Release

# 4. 安装依赖文件
cmake --install .
```

### CMakeLists.txt配置

项目使用模块化的CMake配置，主要包含以下组件：

```cmake
# 核心组件
- utilities: 日志、更新等工具库
- algorithm: 推理算法库  
- transfer: 数据传输库
- ui: 用户界面库
- oscpack: OSC协议库
```

### Qt6模块依赖

```cmake
find_package(Qt6 COMPONENTS 
    Core Gui Widgets 
    Network WebSockets SerialPort 
    REQUIRED
)
```

## 许可证信息

### 主项目许可

本项目采用 **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License** (CC BY-NC-SA 4.0) 进行许可。

#### 许可特点

**✅ 您可以**
- **共享**：在任何媒介以任何形式复制、发布和分发本作品
- **演绎**：修改、变换或以本作品为基础进行创作
- **个人使用**：用于个人学习、研究和非商业目的

**📋 您必须遵守的条件**
- **署名**：必须给出适当的署名，提供指向本许可协议的链接，同时标明是否对原始作品作了修改
- **非商业性使用**：不得将本作品用于商业目的
- **相同方式共享**：如果您修改、变换或以本作品为基础进行创作，您必须基于与原始作品相同的许可协议分发您贡献的作品

**❌ 限制**
- **禁止商业使用**：不得用于商业目的，包括但不限于：
  - 销售本软件或基于本软件的产品
  - 集成到商业硬件产品中
  - 提供基于本软件的付费服务
  - 在商业环境中使用（除非用于研究目的）

### 第三方组件许可

#### Project Babble 2.0.7 组件
```
来源：Project Babble v2.0.7
许可证：Babble Software Distribution License 1.0
使用范围：面部表情识别模型
使用限制：非商业使用（与本项目许可证一致）
项目地址：https://github.com/Project-Babble/ProjectBabble
```

#### EyeTrackVR (ETVR) 组件
```
来源：EyeTrackVR 早期开源版本
许可证：MIT License
使用范围：眼部追踪算法和相关代码
使用时间：基于许可证更新前的开源版本
项目地址：https://github.com/EyeTrackVR
```

#### 其他依赖库
```
- OpenCV：BSD 3-Clause License
- ONNX Runtime：MIT License  
- Qt6：GPL/LGPL双重许可
- libcurl：MIT License
- OSCPack：自定义开源许可
```

完整的第三方组件信息请参考 [THIRD-PARTY.txt](THIRD-PARTY.txt)。

## 商业使用说明

### 非商业许可限制

本项目采用 CC BY-NC-SA 4.0 许可证，**严格禁止任何形式的商业使用**，包括但不限于：

- ❌ **软件销售**：不得销售本软件或包含本软件的产品
- ❌ **硬件集成**：不得将本软件集成到商业硬件产品中
- ❌ **商业服务**：不得基于本软件提供付费服务
- ❌ **企业使用**：不得在商业环境中使用（研究用途除外）

### 适用的使用场景

**✅ 允许的使用**
- 个人学习和娱乐
- 学术研究和教育
- 非营利组织的项目
- 开源项目的开发
- VR爱好者的个人项目

### 商业授权

如果您需要将本项目用于商业目的，请通过以下方式联系我们获得特殊商业许可：
- GitHub Issues：说明您的商业使用需求
- 项目主页：查看联系方式

## 贡献指南

### 参与开发

我们欢迎各种形式的非商业贡献：

1. **问题反馈**：通过 [GitHub Issues](../../issues) 报告bug或建议新功能
2. **代码贡献**：提交Pull Request改进代码质量
3. **文档完善**：帮助改进项目文档和使用指南
4. **测试支持**：在不同环境下测试并反馈兼容性问题

### 贡献者许可协议

通过向本项目贡献代码，您同意：
- 您的贡献将基于 CC BY-NC-SA 4.0 许可证发布
- 您拥有贡献内容的合法权利
- 您的贡献不会违反任何第三方权利

### 代码规范

#### C++编码标准
```cpp
// 1. 使用C++20标准特性
// 2. 遵循RAII原则
// 3. 优先使用智能指针
// 4. 统一的命名规范（snake_case）
```

#### 提交要求
- 确保代码能在MSVC 2022下编译通过
- 包含适当的单元测试
- 遵循现有的代码风格
- 提供清晰的提交信息

## 性能优化

### 系统调优建议

```
1. CPU优化
   - 启用AVX2指令集支持
   - 设置高性能电源计划
   - 关闭不必要的后台程序

2. 内存优化  
   - 确保有足够的可用RAM
   - 考虑使用专用显卡进行推理加速

3. 网络优化（ESP32模式）
   - 减少网络延迟和丢包
```


## 联系方式

- **问题反馈**：[GitHub Issues](../../issues)
- **功能建议**：[GitHub Discussions](../../discussions)
- **技术交流**：请通过Issues进行
- **商业授权**：请通过项目主页联系方式洽谈

## 致谢

- **Project Babble团队**：提供优秀的面部追踪算法和模型
- **EyeTrackVR团队**：提供开源的眼部追踪技术
- **Qt社区**：提供强大的跨平台GUI框架
- **OpenCV团队**：提供计算机视觉基础库
- **所有贡献者**：感谢每一位为项目做出贡献的开发者

---

<div align="center">

**本项目仅供非商业使用 | 基于 CC BY-NC-SA 4.0 许可证发布**

**如果这个项目对您有帮助，请考虑给我们一个Star ⭐**

[⬆️ 回到顶部](#papertracker)

</div>