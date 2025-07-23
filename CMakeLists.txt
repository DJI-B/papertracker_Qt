cmake_minimum_required(VERSION 3.30)
project(PaperTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
带我去打球我去我带我去我的委屈
if (MSVC)
    # 设置预处理器标志
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")
    
    # 根据构建类型设置正确的运行时库
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
        add_definitions(-DDEBUG)
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    endif()
    
    # 确保包含标准库路径
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

# 调试资源文件路径
message(STATUS "资源文件路径: ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
message(STATUS "模型文件路径: ${CMAKE_CURRENT_SOURCE_DIR}/model/eye_model.onnx")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
    message(STATUS "resources.qrc 文件存在")
else()
    message(WARNING "resources.qrc 文件不存在!")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/model/eye_model.onnx")
    message(STATUS "eye_model.onnx 文件存在")
else()
    message(WARNING "eye_model.onnx 文件不存在!")
endif()

# set 3rd party lib path

# Qt installation path - configurable option
set(QT_INSTALL_PATH "D:/QtNew/6.8.2/msvc2022_64" CACHE PATH "Path to Qt installation directory")

if(EXISTS "${QT_INSTALL_PATH}")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${QT_INSTALL_PATH}")
else()
    message(WARNING "Qt installation path not found: ${QT_INSTALL_PATH}")
endif()

## download dependencies
include(FetchContent)

# download opencv
set(OPENCV_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/opencv)
set(OPENCV_CMAKE_FILE ${OPENCV_INSTALL_PATH}/build/x64/vc16/lib/OpenCVConfig.cmake)
set(OPENCV_VERSION 4.11.0)
if(NOT EXISTS ${OPENCV_CMAKE_FILE})
    message(STATUS "OpenCV not found, downloading...")
    
    FetchContent_Declare(
        prebuilt_opencv
        URL https://github.com/opencv/opencv/releases/download/${OPENCV_VERSION}/opencv-${OPENCV_VERSION}-windows.exe
        SOURCE_DIR ${OPENCV_INSTALL_PATH}
    )
    
    FetchContent_MakeAvailable(prebuilt_opencv)
else()
    message(STATUS "OpenCV already exists at: ${OPENCV_INSTALL_PATH}")
endif()
set(OpenCV_DIR ${OPENCV_INSTALL_PATH}/build/x64/vc16/lib)
find_package(OpenCV REQUIRED)

# download onnxruntime
set(ONNXRUNTIME_VERSION 1.22.0)
set(ONNXRUNTIME_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/onnxruntime)
set(ONNXRUNTIME_HEADER_FILE ${ONNXRUNTIME_INSTALL_PATH}/include/onnxruntime_cxx_api.h)
find_package(CUDA)
if(CUDA_FOUND)
    message("CUDA found")
    add_definitions(-DUSE_CUDA)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
    if(NOT EXISTS ${ONNXRUNTIME_HEADER_FILE})
        message(STATUS "ONNXRUNTIME not found, downloading...")
        FetchContent_Declare(
            onnxruntime
            URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-win-x64-gpu-${ONNXRUNTIME_VERSION}.zip
            SOURCE_DIR ${ONNXRUNTIME_INSTALL_PATH}
        )
        FetchContent_MakeAvailable(onnxruntime)
        message(STATUS "ONNXRUNTIME download completed")
    else()
        message(STATUS "ONNXRUNTIME already exists at: ${ONNXRUNTIME_INSTALL_PATH}")
    endif()
else()
    message("CUDA no found")
    if(NOT EXISTS ${ONNXRUNTIME_HEADER_FILE})
        message(STATUS "ONNXRUNTIME not found, downloading...")
        FetchContent_Declare(
            onnxruntime
            URL https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip
            SOURCE_DIR ${ONNXRUNTIME_INSTALL_PATH}
        )
        FetchContent_MakeAvailable(onnxruntime)
        message(STATUS "ONNXRUNTIME download completed")
    else()
        message(STATUS "ONNXRUNTIME already exists at: ${ONNXRUNTIME_INSTALL_PATH}")
    endif()
endif ()
set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime)

# download oscpack
include(ExternalProject)
set(OSCPACK_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/oscpack)
set(SOURCE_DIR ${OSCPACK_PREFIX}/src)
set(BUILD_DIR ${OSCPACK_PREFIX}/build)

function(build_oscpack_both_configs)
    if(NOT EXISTS ${SOURCE_DIR})
        message(STATUS "Downloading oscpack...")
        execute_process(
            COMMAND git clone --depth 1 https://github.com/RossBencina/oscpack.git ${SOURCE_DIR}
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to download oscpack")
        endif()
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(target_lib "${BUILD_DIR}/Debug/oscpack.lib")
        set(build_config "Debug")
    else()
        set(target_lib "${BUILD_DIR}/Release/oscpack.lib")
        set(build_config "Release")
    endif()
    
    if(NOT EXISTS ${target_lib})
        message(STATUS "Building oscpack in ${build_config} mode...")
        
        file(MAKE_DIRECTORY ${BUILD_DIR})
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} -S ${SOURCE_DIR} -B ${BUILD_DIR} 
                -DCMAKE_BUILD_TYPE=${build_config}
                -DOSC_BUILD_TESTS=OFF
                -DOSC_BUILD_EXAMPLES=OFF
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to configure oscpack")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --config Debug
            RESULT_VARIABLE result_debug
        )
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --config Release
            RESULT_VARIABLE result_release
        )
        
        if(NOT result_debug EQUAL 0 AND NOT result_release EQUAL 0)
            message(FATAL_ERROR "Failed to build oscpack")
        endif()
        
        message(STATUS "oscpack built successfully")
    endif()
endfunction()

build_oscpack_both_configs()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OSCPACK_LIBRARY "${BUILD_DIR}/Debug/oscpack.lib")
else()
    set(OSCPACK_LIBRARY "${BUILD_DIR}/Release/oscpack.lib")
endif()

if(NOT EXISTS ${OSCPACK_LIBRARY})
    message(FATAL_ERROR "oscpack library not found: ${OSCPACK_LIBRARY}")
endif()

add_library(oscpack::oscpack STATIC IMPORTED)
set_target_properties(oscpack::oscpack PROPERTIES
    IMPORTED_LOCATION ${OSCPACK_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${SOURCE_DIR}
)
if(WIN32)
    target_link_libraries(oscpack::oscpack INTERFACE ws2_32 winmm)
endif()

message(STATUS "Using oscpack library: ${OSCPACK_LIBRARY}")

# download esptools
set(ESPTOOL_INSTALL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/esptools)

if(NOT EXISTS ${ESPTOOL_INSTALL_PATH}/esptool.exe)
    message(STATUS "Downloading esptool...")

    FetchContent_Declare(
        esptool
        URL https://github.com/espressif/esptool/releases/download/v5.0.0/esptool-v5.0.0-windows-amd64.zip
        SOURCE_DIR ${ESPTOOL_INSTALL_PATH}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    
    FetchContent_MakeAvailable(esptool)
    message(STATUS "esptool downloaded and extracted to: ${ESPTOOL_INSTALL_PATH}")
else()
    message(STATUS "esptool already exists at: ${ESPTOOL_INSTALL_PATH}")
endif()

set(ESPTOOL_EXECUTABLE ${ESPTOOL_INSTALL_PATH}/esptool.exe)

if(EXISTS ${ESPTOOL_EXECUTABLE})
    message(STATUS "esptool executable: ${ESPTOOL_EXECUTABLE}")
else()
    message(WARNING "esptool executable not found at: ${ESPTOOL_EXECUTABLE}")
endif()


find_package(Qt6 COMPONENTS Core Gui Widgets Network WebSockets SerialPort REQUIRED)

if(MSVC)
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Update if necessary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()

############### utilities ################
add_library(
        utilities
        utilities/logger.cpp
        utilities/updater.cpp
)

target_link_libraries(
        utilities PUBLIC
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::SerialPort
)

target_include_directories(
        utilities PUBLIC
        utilities/include
)

############### algorithm ################
add_library(
        algorithm
        algorithm/face_inference.cpp
        algorithm/kalman_fliter.cpp
        algorithm/eye_inference.cpp
        algorithm/base_inference.cpp
)

target_include_directories(
        algorithm
        PUBLIC
        algorithm/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
)

target_link_libraries(
        algorithm
        PUBLIC
        ${OpenCV_LIBS}
        onnxruntime
        utilities
)

# Add CUDA support if available
if(USE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(algorithm PUBLIC CUDA::cudart)
    # 确保链接CUDA提供者库
    target_link_libraries(algorithm PUBLIC onnxruntime_providers_cuda)
    target_link_libraries(algorithm PUBLIC onnxruntime_providers_shared)
endif()

############### transfer ################
add_library(
        transfer
        transfer/serial.cpp
        transfer/video_reader.cpp
        transfer/osc.cpp
        transfer/image_downloader.cpp
        transfer/http_server.cpp
)

target_include_directories(
        transfer
        PUBLIC
        transfer/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack/src
)

target_link_libraries(
        transfer
        PUBLIC
        Qt6::Core Qt6::WebSockets  Qt6::Network Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack::oscpack
        utilities
        Setupapi
        User32
)

# Add CUDA support for transfer if available
if(USE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(transfer PUBLIC onnxruntime_providers_cuda)
    target_link_libraries(transfer PUBLIC onnxruntime_providers_shared)
endif()

############### ui ################
add_library(
        ui
        ui/roi_event.cpp
        ui/face_tracker_window.cpp
        ui/face_tracker_window.ui
        ui/main_window.cpp
        ui/main_window.ui
        ui/eye_tracker_window.cpp
        ui/eye_tracker_window.ui
        ui/translator_manager.cpp
        ui/include/translator_manager.h
)

target_include_directories(
        ui
        PUBLIC
        ui/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack/src
)

target_link_libraries(
        ui
        PUBLIC
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack::oscpack
        transfer
        algorithm
        utilities
)

# Add CUDA support for ui if available
if(USE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(ui PUBLIC onnxruntime_providers_cuda)
    target_link_libraries(ui PUBLIC onnxruntime_providers_shared)
endif()

############### executable ################
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_executable(PaperTracker main.cpp resources.qrc)
else ()
    add_executable(PaperTracker WIN32 main.cpp resources.qrc)
endif ()

target_include_directories(
        PaperTracker PUBLIC
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack/src
)

target_link_directories(
        PaperTracker PUBLIC
        ${ONNXRUNTIME_ROOT}/lib
)

target_link_libraries(PaperTracker PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Qt6::SerialPort
        onnxruntime
        oscpack::oscpack
        ${LIBS}
        ${OpenCV_LIBS}
        #add windows library
        Setupapi
        User32
        utilities
        ui
        algorithm
        transfer
)

# Add CUDA support for main executable if available
if(USE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(PaperTracker PRIVATE onnxruntime_providers_cuda)
    target_link_libraries(PaperTracker PRIVATE onnxruntime_providers_shared)
    target_link_libraries(PaperTracker PRIVATE CUDA::cudart)
    
    # Copy cuDNN DLLs to the build directory
    set(CUDNN_ROOT "C:/Program Files/NVIDIA/CUDNN/v9.10")
    # 根据CUDA版本选择合适的cuDNN二进制文件
    if(CUDAToolkit_VERSION_MAJOR EQUAL 12)
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/12.9")
    else()
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/11.8")
    endif()
    
    message(STATUS "Using cuDNN binaries from: ${CUDNN_BIN_DIR}")
    
    # 复制所有必需的cuDNN DLL文件
    file(GLOB CUDNN_DLLS "${CUDNN_BIN_DIR}/*.dll")
    foreach(CUDNN_DLL ${CUDNN_DLLS})
        get_filename_component(DLL_NAME ${CUDNN_DLL} NAME)
        add_custom_command(TARGET PaperTracker POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CUDNN_DLL}"
            "$<TARGET_FILE_DIR:PaperTracker>/${DLL_NAME}"
            COMMENT "Copying ${DLL_NAME} to build directory")
    endforeach()
endif()

# install dir model to the same dir as the executable
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/opencv/build/x64/vc16/bin/  DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/esptools/  DESTINATION ${CMAKE_BINARY_DIR})
#install(DIRECTORY "D:/QtMVSC/6.8.2/msvc2022_64/bin/"  DESTINATION ${CMAKE_BINARY_DIR})
if (USE_CUDA AND CUDAToolkit_FOUND)
    
    # Install cuDNN DLLs
    set(CUDNN_ROOT "C:/Program Files/NVIDIA/CUDNN/v9.10")
    if(CUDAToolkit_VERSION_MAJOR EQUAL 12)
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/12.9")
    else()
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/11.8")
    endif()
    install(DIRECTORY ${CUDNN_BIN_DIR}/ DESTINATION ${CMAKE_BINARY_DIR} FILES_MATCHING PATTERN "*.dll")
endif ()
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime/lib/  DESTINATION ${CMAKE_BINARY_DIR})

if (WIN32)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif()
    
    # Debug information
    message(STATUS "Qt DLL copy: QT_INSTALL_PATH = ${QT_INSTALL_PATH}")
    message(STATUS "Qt DLL copy: DEBUG_SUFFIX = ${DEBUG_SUFFIX}")
    message(STATUS "Qt DLL copy: Target file dir = $<TARGET_FILE_DIR:${PROJECT_NAME}>")
    
    # Copy Qt platform plugin
    if(EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        message(STATUS "Copying Qt platform plugin from: ${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    else()
        message(WARNING "Qt platform plugin not found: ${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
    endif()
    
    # Copy Qt core libraries
    foreach(QT_LIB Core Gui Widgets Network WebSockets SerialPort)
        if(EXISTS "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll")
            message(STATUS "Copying Qt library: Qt6${QT_LIB}${DEBUG_SUFFIX}.dll")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        else()
            message(WARNING "Qt library not found: ${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll")
        endif()
    endforeach(QT_LIB)
endif()