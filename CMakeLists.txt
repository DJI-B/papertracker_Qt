cmake_minimum_required(VERSION 3.30)

project(PaperTracker 
    VERSION 1.0.0 
    DESCRIPTION "Paper Tracker Application"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 编译器特定设置
if(MSVC)
    # 设置预处理器标志和警告等级
    add_compile_options(
        /Zc:preprocessor     # 启用标准预处理器
        /utf-8               # 设置UTF-8编码
        /W4                  # 最高警告级别
        /permissive-         # 禁用非标准扩展
    )
    
    # 根据构建类型设置运行时库
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Debug模式添加调试定义
    add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)
endif()

# ============================================================================
# 选项和缓存变量
# ============================================================================
option(USE_CUDA "Enable CUDA support" OFF)
option(BUILD_TESTS "Build test executables" OFF)

# Qt路径设置（支持环境变量覆盖）
set(QT_ROOT_PATH "" CACHE PATH "Qt installation root path")
if(NOT QT_ROOT_PATH)
    if(DEFINED ENV{QT_ROOT})
        set(QT_ROOT_PATH $ENV{QT_ROOT})
    else()
        # 尝试常见Qt安装位置
        set(QT_CANDIDATES 
            "D:/QtNew/6.8.2/msvc2022_64"
            "E:/Qt/6.8.2/msvc2022_64"
            "C:/Qt/6.8.2/msvc2022_64"
        )
        foreach(candidate ${QT_CANDIDATES})
            if(EXISTS "${candidate}")
                set(QT_ROOT_PATH "${candidate}")
                break()
            endif()
        endforeach()
    endif()
endif()

# 设置运行时依赖项（在函数定义之后调用）
#setup_runtime_dependencies(PaperTracker)

if(QT_ROOT_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${QT_ROOT_PATH}")
    message(STATUS "使用Qt路径: ${QT_ROOT_PATH}")
else()
    message(WARNING "未找到Qt安装路径，请设置QT_ROOT_PATH变量或环境变量QT_ROOT")
endif()

# ============================================================================
# 第三方库路径设置
# ============================================================================
set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/3rdParty)

# CURL
set(CURL_DIR ${THIRD_PARTY_ROOT}/curl-install/lib/cmake/CURL)

# OpenCV
set(OpenCV_DIR ${THIRD_PARTY_ROOT}/opencv-mvsc/opencv/build)

# ONNX Runtime路径设置
if(USE_CUDA)
    find_package(CUDA QUIET)
    if(CUDA_FOUND)
        message(STATUS "启用CUDA支持")
        add_compile_definitions(USE_CUDA)
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
        set(ONNXRUNTIME_ROOT ${THIRD_PARTY_ROOT}/onnxruntime-win-x64-gpu-1.20.1)
    else()
        message(WARNING "未找到CUDA，使用CPU版本ONNX Runtime")
        set(ONNXRUNTIME_ROOT ${THIRD_PARTY_ROOT}/onnxruntime-win-x64-1.20.1)
    endif()
else()
    set(ONNXRUNTIME_ROOT ${THIRD_PARTY_ROOT}/onnxruntime-win-x64-1.20.1)
endif()

# ============================================================================
# 查找依赖包
# ============================================================================
find_package(OpenCV CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    WebSockets 
    SerialPort
)

# ============================================================================
# 资源文件验证函数
# ============================================================================
function(verify_resource_files)
    set(REQUIRED_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/models.qrc"
    )
    
    # 验证QRC文件中引用的模型文件
    set(MODEL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/model/eye_model.onnx"
        "${CMAKE_CURRENT_SOURCE_DIR}/model/face_model.onnx"
    )
    
    foreach(file ${REQUIRED_FILES})
        if(EXISTS "${file}")
            message(STATUS "✓ QRC资源文件存在: ${file}")
        else()
            message(WARNING "✗ QRC资源文件缺失: ${file}")
        endif()
    endforeach()
    
    foreach(file ${MODEL_FILES})
        if(EXISTS "${file}")
            message(STATUS "✓ 模型文件存在(将通过QRC嵌入): ${file}")
        else()
            message(WARNING "✗ 模型文件缺失(QRC编译将失败): ${file}")
        endif()
    endforeach()
endfunction()

# ============================================================================
# 运行时依赖设置函数
# ============================================================================
function(setup_runtime_dependencies target_name)
    # 创建运行时目录结构
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${target_name}>/resources"
    )
    
    # 模型文件已通过QRC资源系统嵌入，无需复制
    # 注意：模型文件通过 models.qrc 编译到可执行文件中
    
    # 复制OpenCV DLL
    set(OPENCV_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build/x64/vc16/bin")
    if(EXISTS "${OPENCV_DLL_PATH}/")
        file(GLOB OPENCV_DLLS "${OPENCV_DLL_PATH}/*.dll")
        foreach(dll ${OPENCV_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制CURL DLL
    set(CURL_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/curl-install/bin")
    if(EXISTS "${CURL_DLL_PATH}/")
        file(GLOB CURL_DLLS "${CURL_DLL_PATH}/*.dll")
        foreach(dll ${CURL_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制ONNX Runtime DLL
    if(USE_CUDA AND CUDA_FOUND)
        set(ONNX_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.20.1/lib")
    else()
        set(ONNX_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.20.1/lib")
    endif()
    
    if(EXISTS "${ONNX_DLL_PATH}/")
        file(GLOB ONNX_DLLS "${ONNX_DLL_PATH}/*.dll")
        foreach(dll ${ONNX_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制ESP工具（如果存在）
    if(EXISTS "${CMAKE_SOURCE_DIR}/3rdParty/esptools/")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/3rdParty/esptools"
            "$<TARGET_FILE_DIR:${target_name}>/esptools"
        )
    endif()
    
    # 复制资源文件
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:${target_name}>/resources"
        )
    endif()
    
    message(STATUS "已配置运行时依赖项复制到: $<TARGET_FILE_DIR:${target_name}>")
endfunction()

# ============================================================================
# 安装函数（用于发布）
# ============================================================================
function(install_for_distribution)
    install(TARGETS PaperTracker
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    # 模型文件已通过QRC嵌入，无需安装
    
    # 安装资源文件
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/")
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
            DESTINATION bin/resources
        )
    endif()
    
    message(STATUS "已配置发布安装，安装路径: ${CMAKE_INSTALL_PREFIX}")
endfunction()

# 执行资源文件验证
verify_resource_files()

# 平台特定设置
if(WIN32)
    set(IP_SYSTEM_TYPE_PATH ip/win32)
    set(PLATFORM_LIBS Ws2_32 winmm Setupapi User32)
else()
    set(IP_SYSTEM_TYPE_PATH ip/posix)
    set(PLATFORM_LIBS)
endif()

# ============================================================================
# 资源文件验证
# ============================================================================
function(verify_resource_files)
    set(REQUIRED_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/models.qrc"
    )
    
    # 验证QRC文件中引用的模型文件
    set(MODEL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/model/eye_model.onnx"
        "${CMAKE_CURRENT_SOURCE_DIR}/model/face_model.onnx"
    )
    
    foreach(file ${REQUIRED_FILES})
        if(EXISTS "${file}")
            message(STATUS "✓ QRC资源文件存在: ${file}")
        else()
            message(WARNING "✗ QRC资源文件缺失: ${file}")
        endif()
    endforeach()
    
    foreach(file ${MODEL_FILES})
        if(EXISTS "${file}")
            message(STATUS "✓ 模型文件存在(将通过QRC嵌入): ${file}")
        else()
            message(WARNING "✗ 模型文件缺失(QRC编译将失败): ${file}")
        endif()
    endforeach()
endfunction()

verify_resource_files()

# ============================================================================
# 创建库目标
# ============================================================================

# Utilities库
add_library(utilities
    utilities/logger.cpp
    utilities/updater.cpp
)

target_include_directories(utilities 
    PUBLIC utilities/include
)

target_link_libraries(utilities 
    PUBLIC 
        Qt6::Core 
        Qt6::Gui 
        Qt6::Widgets 
        Qt6::Network 
        Qt6::WebSockets 
        Qt6::SerialPort 
        CURL::libcurl
)

# OSC Pack库
add_library(oscpack
    3rdParty/oscpack/ip/IpEndpointName.cpp
    3rdParty/oscpack/${IP_SYSTEM_TYPE_PATH}/NetworkingUtils.cpp
    3rdParty/oscpack/${IP_SYSTEM_TYPE_PATH}/UdpSocket.cpp
    3rdParty/oscpack/osc/OscTypes.cpp
    3rdParty/oscpack/osc/OscReceivedElements.cpp
    3rdParty/oscpack/osc/OscPrintReceivedElements.cpp
    3rdParty/oscpack/osc/OscOutboundPacketStream.cpp
)

target_include_directories(oscpack 
    PUBLIC ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

# Algorithm库
add_library(algorithm
    algorithm/face_inference.cpp
    algorithm/kalman_fliter.cpp
    algorithm/eye_inference.cpp
    algorithm/base_inference.cpp
)

target_include_directories(algorithm 
    PUBLIC 
        algorithm/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
)

target_link_libraries(algorithm 
    PUBLIC 
        ${OpenCV_LIBS}
        onnxruntime
        utilities
)

# Transfer库
add_library(transfer
    transfer/serial.cpp
    transfer/video_reader.cpp
    transfer/osc.cpp
    transfer/image_downloader.cpp
    transfer/http_server.cpp
)

target_include_directories(transfer 
    PUBLIC 
        transfer/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

target_link_libraries(transfer 
    PUBLIC 
        Qt6::Core 
        Qt6::WebSockets  
        Qt6::Network 
        Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack
        CURL::libcurl
        utilities
        ${PLATFORM_LIBS}
)

# UI库
add_library(ui
    ui/roi_event.cpp
    ui/face_tracker_window.cpp
    ui/face_tracker_window.ui
    ui/main_window.cpp
    ui/main_window.ui
    ui/eye_tracker_window.cpp
    ui/eye_tracker_window.ui
)

target_include_directories(ui 
    PUBLIC 
        ui/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

target_link_libraries(ui 
    PUBLIC 
        Qt6::Core 
        Qt6::Gui 
        Qt6::Widgets 
        Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack
        transfer
        algorithm
        CURL::libcurl
        utilities
)

# ============================================================================
# 主执行文件
# ============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(PaperTracker main.cpp models.qrc)
else()
    add_executable(PaperTracker WIN32 main.cpp models.qrc)
endif()

target_include_directories(PaperTracker 
    PRIVATE 
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

target_link_directories(PaperTracker 
    PRIVATE ${ONNXRUNTIME_ROOT}/lib
)

target_link_libraries(PaperTracker 
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Qt6::SerialPort
        onnxruntime
        oscpack
        CURL::libcurl
        ${OpenCV_LIBS}
        ${PLATFORM_LIBS}
        utilities
        ui
        algorithm
        transfer
)

# 设置运行时依赖项
setup_runtime_dependencies(PaperTracker)

# ============================================================================
# 安装和部署配置
# ============================================================================

# 设置默认安装前缀为构建目录下的install文件夹
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "安装目录" FORCE)
endif()

# 开发阶段使用POST_BUILD复制文件，而不是install
function(setup_runtime_dependencies target_name)
    # 创建运行时目录结构
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${target_name}>/model"
        "$<TARGET_FILE_DIR:${target_name}>/resources"
    )
    
    # 模型文件已通过QRC资源系统嵌入，无需复制
    # 注意：模型文件通过 models.qrc 编译到可执行文件中
    
    # 复制OpenCV DLL
    set(OPENCV_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build/x64/vc16/bin")
    if(EXISTS "${OPENCV_DLL_PATH}/")
        file(GLOB OPENCV_DLLS "${OPENCV_DLL_PATH}/*.dll")
        foreach(dll ${OPENCV_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制CURL DLL
    set(CURL_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/curl-install/bin")
    if(EXISTS "${CURL_DLL_PATH}/")
        file(GLOB CURL_DLLS "${CURL_DLL_PATH}/*.dll")
        foreach(dll ${CURL_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制ONNX Runtime DLL
    if(USE_CUDA AND CUDA_FOUND)
        set(ONNX_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.20.1/lib")
    else()
        set(ONNX_DLL_PATH "${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.20.1/lib")
    endif()
    
    if(EXISTS "${ONNX_DLL_PATH}/")
        file(GLOB ONNX_DLLS "${ONNX_DLL_PATH}/*.dll")
        foreach(dll ${ONNX_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${target_name}>"
            )
        endforeach()
    endif()
    
    # 复制ESP工具（如果存在）
    if(EXISTS "${CMAKE_SOURCE_DIR}/3rdParty/esptools/")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/3rdParty/esptools"
            "$<TARGET_FILE_DIR:${target_name}>/esptools"
        )
    endif()
    
    # 复制资源文件
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:${target_name}>/resources"
        )
    endif()
    
    message(STATUS "已配置运行时依赖项复制到: $<TARGET_FILE_DIR:${target_name}>")
endfunction()

# 可选的安装功能（用于发布）
function(install_for_distribution)
    install(TARGETS PaperTracker
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    # 模型文件已通过QRC嵌入，无需安装
    
    # 安装资源文件
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/")
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
            DESTINATION bin/resources
        )
    endif()
    
    message(STATUS "已配置发布安装，安装路径: ${CMAKE_INSTALL_PREFIX}")
endfunction()

# 根据是否是发布构建来决定使用哪种方式
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ENABLE_INSTALL)
    install_for_distribution()
endif()

# ============================================================================
# Qt部署
# ============================================================================
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_SOURCE_DIR}/cmake/QtDeploy.cmake OPTIONAL)
    if(COMMAND qt_deploy_runtime)
        qt_deploy_runtime(PaperTracker)
    else()
        # 回退到手动Qt部署
        include(${CMAKE_SOURCE_DIR}/cmake/ManualQtDeploy.cmake OPTIONAL)
    endif()
endif()

# ============================================================================
# 开发辅助功能
# ============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # 生成compile_commands.json用于IDE
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    # 添加调试信息
    message(STATUS "=== 构建配置信息 ===")
    message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
    message(STATUS "编译器: ${CMAKE_CXX_COMPILER}")
    message(STATUS "Qt路径: ${QT_ROOT_PATH}")
    message(STATUS "OpenCV版本: ${OpenCV_VERSION}")
    message(STATUS "CUDA支持: ${USE_CUDA}")
    message(STATUS "==================")
endif()

# ============================================================================
# 测试支持（可选）
# ============================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests OPTIONAL)
endif()