cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

SET(DEFINES "-DENABLE_MAVLINK -DENABLE_CUSTOM -DENABLE_AERO")    

# Avahi
find_package(Avahi QUIET)
IF(Avahi_FOUND)
    SET(DEFINES "${DEFINES} -DENABLE_AVAHI")
    MESSAGE(STATUS "Found Avahi: ${Avahi_CLIENT_INCLUDE_DIRS}")
ENDIF(Avahi_FOUND)

# Gazebo
find_package(gazebo QUIET)
IF(gazebo_FOUND)
    SET(DEFINES "${DEFINES} -DENABLE_GAZEBO")
    MESSAGE(STATUS "Found Gazebo: ${GAZEBO_LIBRARIES}")
ENDIF(gazebo_FOUND)

# RealSense
FIND_PACKAGE(RealSense QUIET)
IF(RealSense_FOUND)
    SET(DEFINES, "${DEFINES} -DENABLE_REALSENSE")
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

project(dcm)

add_definitions(
    -std=c++11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    ${DEFINES}
)

file(GLOB SRC_FILES *.cpp)

add_subdirectory(Plugins/AeroAtomIspCamera)
add_subdirectory(Plugins/CustomCamera)
add_subdirectory(Plugins/V4l2Camera)

IF(gazebo_FOUND)
add_subdirectory(Plugins/GazeboCamera)
ENDIF(gazebo_FOUND)

IF(RealSense_FOUND)
add_subdirectory(Plugins/RealSenseCamera)
ENDIF(RealSense_FOUND)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

set(SYSTEMD_CONFIG_DIR "/lib/systemd/system" CACHE STRING "Path to the systemd configuration dir")
set(SYSTEMD_UNIT_NAME "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.service")
configure_file("${PROJECT_SOURCE_DIR}/cmake/dronecode-camera-manager.service.in" "${SYSTEMD_UNIT_NAME}")
install(FILES "${SYSTEMD_UNIT_NAME}" DESTINATION "${SYSTEMD_CONFIG_DIR}/" COMPONENT Runtime)

target_link_libraries(${PROJECT_NAME}
    AeroAtomIspCamera
    CustomCamera
    V4l2Camera
)

IF(gazebo_FOUND)
target_link_libraries(${PROJECT_NAME}
    GazeboCamera
    ${GAZEBO_LIBRARIES}
)
ENDIF(gazebo_FOUND)

IF(RealSense_FOUND)
target_link_libraries(${PROJECT_NAME}
    RealSenseCamera
    ${RealSense_LIBRARIES}
)
ENDIF(RealSense_FOUND)

IF(Avahi_FOUND)
target_link_libraries(${PROJECT_NAME}
    Avahi::client
    Avahi::glib
)
ENDIF(Avahi_FOUND)

target_link_libraries(${PROJECT_NAME}
    pthread
    ${GST_LIBRARIES}
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
