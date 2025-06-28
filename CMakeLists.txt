cmake_minimum_required(VERSION 3.30)
project(PaperTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /Zc:preprocessor")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MTd")
    add_definitions(-DDEBUG)
endif ()


# set 3rd party lib path

# Qt installation path - configurable option
set(QT_INSTALL_PATH "D:/QtNew/6.8.2/msvc2022_64" CACHE PATH "Path to Qt installation directory")

if(EXISTS "${QT_INSTALL_PATH}")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${QT_INSTALL_PATH}")
else()
    message(WARNING "Qt installation path not found: ${QT_INSTALL_PATH}")
endif()

set(CURL_DIR ${CMAKE_SOURCE_DIR}/3rdParty/curl-install/lib/cmake/CURL)
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build)

# CUDA configuration
option(USE_CUDA "Enable CUDA support" ON)
if(USE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit)
    if(CUDAToolkit_FOUND)
        message(STATUS "CUDA found. Version: ${CUDAToolkit_VERSION}")
        message(STATUS "CUDA Toolkit Root: ${CUDAToolkit_ROOT}")
        message(STATUS "CUDA Libraries: ${CUDAToolkit_LIBRARY_DIR}")
        add_definitions(-DUSE_CUDA)
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
        set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.22.0)
        message(STATUS "Using CUDA ONNX Runtime: ${ONNXRUNTIME_ROOT}")
    else()
        message(STATUS "CUDA not found, falling back to CPU version")
        set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.22.0)
        message(STATUS "Using CPU ONNX Runtime: ${ONNXRUNTIME_ROOT}")
    endif()
else()
    message(STATUS "CUDA support disabled")
    set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.22.0)
    message(STATUS "Using CPU ONNX Runtime: ${ONNXRUNTIME_ROOT}")
endif()

find_package(OpenCV CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets Network WebSockets SerialPort REQUIRED)

IF(WIN32)
    set(IpSystemTypePath ip/win32)
    set(LIBS ${LIBS} Ws2_32 winmm)
ELSE(WIN32)
    set(IpSystemTypePath ip/posix)
ENDIF(WIN32)

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
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::SerialPort CURL::libcurl
)

target_include_directories(
        utilities PUBLIC
        utilities/include
)


############### osc pack ################
add_library(oscpack
        3rdParty/oscpack/ip/IpEndpointName.cpp
        3rdParty/oscpack/${IpSystemTypePath}/NetworkingUtils.cpp
        3rdParty/oscpack/${IpSystemTypePath}/UdpSocket.cpp
        3rdParty/oscpack/osc/OscTypes.cpp
        3rdParty/oscpack/osc/OscReceivedElements.cpp
        3rdParty/oscpack/osc/OscPrintReceivedElements.cpp
        3rdParty/oscpack/osc/OscOutboundPacketStream.cpp
)

target_include_directories(
        oscpack
        PUBLIC
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
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
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

target_link_libraries(
        transfer
        PUBLIC
        Qt6::Core Qt6::WebSockets  Qt6::Network Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack
        CURL::libcurl
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
)

target_include_directories(
        ui
        PUBLIC
        ui/include
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
)

target_link_libraries(
        ui
        PUBLIC
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SerialPort
        ${OpenCV_LIBS}
        onnxruntime
        oscpack
        transfer
        algorithm
        CURL::libcurl
        utilities
)

# Add CUDA support for ui if available
if(USE_CUDA AND CUDAToolkit_FOUND)
    target_link_libraries(ui PUBLIC onnxruntime_providers_cuda)
    target_link_libraries(ui PUBLIC onnxruntime_providers_shared)
endif()

############### executable ################
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_executable(PaperTracker main.cpp models.qrc)
else ()
    add_executable(PaperTracker WIN32 main.cpp models.qrc)
endif ()

target_include_directories(
        PaperTracker PUBLIC
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_ROOT}/include
        ${CMAKE_SOURCE_DIR}/3rdParty/oscpack
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
        oscpack
        ${LIBS}
        CURL::libcurl
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
install(DIRECTORY ${CMAKE_SOURCE_DIR}/model/  DESTINATION ${CMAKE_BINARY_DIR}/model)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build/x64/vc16/bin/  DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/curl-install/bin/  DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/esptools/  DESTINATION ${CMAKE_BINARY_DIR})
#install(DIRECTORY "D:/QtMVSC/6.8.2/msvc2022_64/bin/"  DESTINATION ${CMAKE_BINARY_DIR})
if (USE_CUDA AND CUDAToolkit_FOUND)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.22.0/lib/  DESTINATION ${CMAKE_BINARY_DIR})
    
    # Install cuDNN DLLs
    set(CUDNN_ROOT "C:/Program Files/NVIDIA/CUDNN/v9.10")
    if(CUDAToolkit_VERSION_MAJOR EQUAL 12)
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/12.9")
    else()
        set(CUDNN_BIN_DIR "${CUDNN_ROOT}/bin/11.8")
    endif()
    install(DIRECTORY ${CUDNN_BIN_DIR}/ DESTINATION ${CMAKE_BINARY_DIR} FILES_MATCHING PATTERN "*.dll")
else ()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.22.0/lib/  DESTINATION ${CMAKE_BINARY_DIR})
endif ()
# install resources
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

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