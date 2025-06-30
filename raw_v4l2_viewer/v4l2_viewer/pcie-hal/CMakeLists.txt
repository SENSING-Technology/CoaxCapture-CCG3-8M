cmake_minimum_required(VERSION 2.9)
project(pcie_camera)
include_directories(include)

add_definitions(--std=c++11 -g -W -Wall -Wunused-variable -Wunused-parameter -Wunused-function -Wunused)

set(CMAKE_LIB_DIR ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

add_library(pcie_camera SHARED src/hal_camera.cpp src/pcie_camera.cpp src/camera.cpp  src/device_manager.cpp src/device_manager.cpp src/PcieCamera.cpp src/yuvtobgr.cpp src/watermark.cpp) 
#add_library(pcie_camera SHARED src/hal_camera.cpp src/camera.cpp  src/device_manager.cpp src/device_manager.cpp src/PcieCamera.cpp src/yuvtobgr.cpp)

include_directories(/usr/local/ffmpeg/include/)
link_directories(/usr/local/ffmpeg/lib/)
add_executable(pcie_preview src/main.cpp)
target_link_libraries(pcie_preview pcie_camera pthread avcodec avfilter  avutil swresample avdevice avformat swscale)


#-lavcodec -lavfilter -lavutil -lswresample -lavdevice -lavformat -lpostproc -lswscale -lpthread -lstdc++ -lm -lasound -lavfilter -lfreetype
