cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)
find_package(Avahi REQUIRED)

# RealSense
FIND_PACKAGE(RealSense QUIET)
IF(RealSense_FOUND)
    MESSAGE(STATUS "Found RealSense: ${RealSense_INCLUDE_DIRS}")
ENDIF(RealSense_FOUND)

# RealSense2
#FIND_PACKAGE(RealSense2 QUIET)
#IF(RealSense2_FOUND)
#    MESSAGE(STATUS "Found RealSense2: ${RealSense2_INCLUDE_DIRS}")
#ENDIF(RealSense2_FOUND)

set(GSTREAMER_MINIMUM_VERSION 1.8)
pkg_check_modules(GST1_TEST gstreamer-1.0)

if (NOT GST1_TEST_FOUND )
    message( FATAL_ERROR "GStreamer 1.8 required." )
endif()
if ( ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
    message( FATAL_ERROR "GStreamer 1.8 required. Found ${GST1_TEST_VERSION} instead." )
endif()

pkg_check_modules(GST REQUIRED
    gstreamer-rtsp-1.0
    gstreamer-1.0
    gstreamer-rtsp-server-1.0
    gstreamer-app-1.0
)

project(CameraManager)

add_definitions(
    -std=c++11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -DENABLE_GAZEBO
    -DENABLE_MAVLINK
    -DENABLE_AVAHI
    -DENABLE_REALSENSE
    -DENABLE_CUSTOM
    -DENABLE_AERO
)

file(GLOB SRC_FILES *.cpp)

add_subdirectory(Plugins/AeroAtomIspCamera)
add_subdirectory(Plugins/CustomCamera)
add_subdirectory(Plugins/GazeboCamera)
add_subdirectory(Plugins/RealSenseCamera)
add_subdirectory(Plugins/V4l2Camera)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
)

target_link_libraries(${PROJECT_NAME}
    AeroAtomIspCamera
    CustomCamera
    GazeboCamera
    RealSenseCamera
    ${RealSense_LIBRARIES}
    V4l2Camera
    Avahi::client
    Avahi::glib
)

include_directories(
    ${GST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
    ${GST_LIBRARY_DIRS}
)

target_compile_options(${PROJECT_NAME} PUBLIC
    ${GST_CFLAGS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/../modules/mavlink/common
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(CameraManager
    pthread
    ${GST_LIBRARIES}
)
