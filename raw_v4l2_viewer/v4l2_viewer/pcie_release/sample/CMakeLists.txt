cmake_minimum_required(VERSION 2.8.3)

set(OS ubuntu20.04)

set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(COMMAND gcc -dumpversion OUTPUT_VARIABLE GCC_VERSION)
# command out will get ENTER at the string end, it must
string(STRIP "${GCC_VERSION}" GCC_VERSION)

string(CONCAT SYS_ENV ${OS} "-" ${CMAKE_SYSTEM_PROCESSOR} "-gcc" ${GCC_VERSION})

#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

add_definitions(--std=c++11 -g -W -Wall
 -Wunused-variable -Wunused-parameter -Wunused-function -Wunused)

#set(MY_PATH "/home/mikey/work/pcie/pcie_release/test/../lib/ubuntu16.04-x86_64-gcc5.4.0")
#set(MY_PATH ${PROJECT_DIR}/../lib/${SYS_ENV})
set(MY_PATH "../../pcie-hal/build")

set(ENC_PATH "../../pcie_encode/build/AppEncCuda")

add_definitions(--std=c++11 -g -W -Wall
 -Wunused-variable -Wunused-parameter -Wunused-function -Wunused)

include_directories(${PROJECT_DIR}/../include ${PROJECT_SOURCE_DIR})

#link_directories(/home/mikey/work/pcie/pcie_release/test/../lib/ubuntu16.04-x86_64-gcc5.4.0)
#link_directories(${PROJECT_DIR}/../lib/${SYS_ENV})
link_directories(${MY_PATH} /usr/local/lib)
set(CMAKE_LIBRARY_PATH ${MY_PATH})

#link_directories(${ENC_PATH} /usr/local/lib)
#set(CMAKE_LIBRARY_PATH ${ENC_PATH})

find_library(MY_LIBRARIES NAMES  pcie_camera PATHS ${MY_PATH})
if(NOT MY_LIBRARIES)
   message("can not find libpcie_camera.so")
else()
endif()

#find_library(ENC_LIBRARIES NAMES  enc_cuda PATHS ${ENC_PATH})
#if(NOT ENC_LIBRARIES)
#message("can not find libenc_cuda.so")
#else()
#endif()

find_file(SDL2_INCLUDE_DIR NAMES SDL.h HINTS SDL)
find_library(SDL2_LIBRARY NAMES SDL2)
if(NOT SDL2_LIBRARY)
   message(search path: ${CMAKE_LIBRARY_PATH})
   message(FATAL_ERROR "can not find libSDL.so")
else()
endif()

find_file(SDL_INCLUDE_DIR NAMES SDL.h HINTS SDL)
find_library(SDL_LIBRARY NAMES SDL)
if(NOT SDL_LIBRARY)
   message(search path: ${CMAKE_LIBRARY_PATH})
   message(FATAL_ERROR "can not find libSDL.so")
else()
endif()


find_file(X11_INCLUDE_DIR NAMES Xlib.h HINTS X11)
find_library(X11_LIBRARY NAMES X11)
if(NOT X11_LIBRARY)
   message(search path: ${CMAKE_LIBRARY_PATH})
   message(FATAL_ERROR "can not find libSDL.so")
else()
endif()

find_library(MY_LIBRARIES NAMES  pcie_camera PATHS ${MY_LIB_PATH})
if(NOT MY_LIBRARIES)
   message(FATAL_ERROR "can not find libpcie_camera.so")
else()
endif()

set(MY_LIBS  pcie_camera  ${SDL_LIBRARY} )

set(OpenCV_DIR /usr/local/opencv)
find_package(OpenCV REQUIRED)
#find_package(CUDA REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
#include_directories(${CUDA_INCLUDE_DIRS})

#
#find_library(NVENCODEAPI_LIB nvidia-encode)
#find_package(CUDA)
#message("CUDA_CUDA_LIBRARY: ${CUDA_CUDA_LIBRARY}")
#message("CUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")
#
#set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
#
#set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")
#if ( CMAKE_COMPILER_IS_GNUCC )
#    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
#        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
#    endif()
#endif()

include_directories(/usr/local/ffmpeg/include/)
link_directories(/usr/local/ffmpeg/lib/)
add_executable(view view.cpp fun.cpp)
#target_link_libraries(view  SDL pcie_camera enc_cuda pthread avcodec avfilter avutil swresample avdevice avformat postproc swscale freetype ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB})
target_link_libraries(view  ${OpenCV_LIBS} SDL X11 pcie_camera pthread avcodec avfilter avutil swresample avdevice avformat swscale)
#target_link_libraries(view  ${OpenCV_LIBS} ${CUDA_CUDA_LIBRARIES} SDL X11 pcie_camera pthread avcodec avfilter avutil swresample avdevice avformat swscale)
#add_executable(picture ./test/player.cpp ../../pcie-hal/src/yuvtobgr.cpp ../../pcie-hal/src/watermark.cpp)
#target_link_libraries(picture ${SDL_LIBRARY} pthread avcodec avfilter avutil swresample avdevice avformat postproc swscale freetype)

