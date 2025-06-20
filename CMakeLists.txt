cmake_minimum_required(VERSION 3.30)
project(PaperTracker)

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

#if develop qt is on set path to the qt path else set the path to the qt path

if (EXISTS "D:/QtNew/6.8.2/msvc2022_64")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/QtNew/6.8.2/msvc2022_64")
else ()
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "E:/Qt/6.8.2/msvc2022_64")
endif ()

set(CURL_DIR ${CMAKE_SOURCE_DIR}/3rdParty/curl-install/lib/cmake/CURL)
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build)
find_package(CUDA)
if(CUDA_FOUND)
    message("CUDA found")
    add_definitions(-DUSE_CUDA)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
    set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.20.1)
else()
    message("CUDA no found")
    set(ONNXRUNTIME_ROOT ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.20.1)
endif ()
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

# install dir model to the same dir as the executable
install(DIRECTORY ${CMAKE_SOURCE_DIR}/model/  DESTINATION ${CMAKE_BINARY_DIR}/model)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/opencv-mvsc/opencv/build/x64/vc16/bin/  DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/curl-install/bin/  DESTINATION ${CMAKE_BINARY_DIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/esptools/  DESTINATION ${CMAKE_BINARY_DIR})
#install(DIRECTORY "D:/QtMVSC/6.8.2/msvc2022_64/bin/"  DESTINATION ${CMAKE_BINARY_DIR})
if (CUDA_FOUND)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-gpu-1.20.1/lib/  DESTINATION ${CMAKE_BINARY_DIR})
else ()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/3rdParty/onnxruntime-win-x64-1.20.1/lib/  DESTINATION ${CMAKE_BINARY_DIR})
endif ()
# install resources
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif()
    endif()
    if(EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif()
    foreach(QT_LIB Core Gui Widgets Network Websockets SerialPort)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(QT_LIB)
endif()