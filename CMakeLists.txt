cmake_minimum_required(VERSION 3.5.1)
project(OAK_MPP_D_E_RTSP)
set(CMAKE_CXX_STANDARD 14)
set(LIBRARY_NAME RtspServer)
file(GLOB HEADER RtspServer/RtspServerWarpper.h)
file(GLOB RTSTSRC_LIST
        "RtspServer/src/net/*.cpp"
        "RtspServer/src/xop/*.cpp"
        "RtspServer/RtspServerWarpper.cpp")
add_library(${LIBRARY_NAME} STATIC
        ${RTSTSRC_LIST}
        )
add_definitions(-fPIC)
find_package(OpenCV REQUIRED)
find_package(depthai CONFIG REQUIRED)
include_directories(${LIBRARY_NAME} PRIVATE RtspServer/src)
include_directories(${LIBRARY_NAME} PRIVATE RtspServer/src/3rdpart)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/utility)
include_directories(${CMAKE_CURRENT_LIST_DIR}/mpp/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR}/mpp/osal/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR}/mpp/utils)
include_directories(${CMAKE_CURRENT_LIST_DIR}/mpp/base/inc)
include_directories(${CMAKE_CURRENT_LIST_DIR})
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/mpp/utils UTIL_LIST) # 搜索当前目录下的所有.cpp文件，存储在变量SRC_LIST中


aux_source_directory(./src SRC_LIST) # 搜索当前目录下的所有.cpp文件，存储在变量SRC_LIST中
add_executable(OAK_MPP_D_E_RTSP main.cpp ${SRC_LIST} ${UTIL_LIST} utility/utility.cpp)

target_link_libraries(OAK_MPP_D_E_RTSP
        rockchip_mpp
        ${OpenCV_LIBS}
        depthai::opencv
        RtspServer
        pthread
        -lavformat
        -lswresample
        -lavcodec
        -lswscale
        -lavutil
        -lz

        )
