cmake_minimum_required(VERSION 3.5)

project(CameraManager)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

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

# Avahi
find_package(Avahi QUIET)
IF(Avahi_FOUND)
    SET(DEFINES "${DEFINES} -DENABLE_AVAHI")
    MESSAGE(STATUS "Found Avahi: ${Avahi_CLIENT_INCLUDE_DIRS}")
ENDIF(Avahi_FOUND)

add_custom_target(tests)
add_dependencies(tests 
    test_camera_parameters
    test_mavlink_protocol
    test_rtsp_udp_stream_discovery
)

add_definitions(
    -std=c++11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    ${DEFINES}
)

add_executable(test_camera_parameters
    test_camera_parameters.cpp
    ../src/glib_mainloop.cpp
    ../src/log.cpp
    ../src/mainloop.cpp
    ../src/socket.cpp
    ../src/pollable.cpp
    ../src/util.cpp
    ../src/CameraParameters.cpp
)
target_include_directories(test_camera_parameters PUBLIC
    ${PROJECT_SOURCE_DIR}/../modules/mavlink/common
    ${PROJECT_SOURCE_DIR}/../src
    ${GST_INCLUDE_DIRS}
)
target_link_libraries(test_camera_parameters
    pthread
    ${GST_LIBRARIES}
)
IF(Avahi_FOUND)
    target_link_libraries(test_camera_parameters
        Avahi::client
        Avahi::glib
    )
ENDIF(Avahi_FOUND)

add_executable(test_mavlink_protocol
    test_mavlink_protocol.cpp
    ../src/glib_mainloop.cpp
    ../src/log.cpp
    ../src/mainloop.cpp
    ../src/socket.cpp
    ../src/pollable.cpp
)
target_include_directories(test_mavlink_protocol PUBLIC
    ${PROJECT_SOURCE_DIR}/../modules/mavlink/common
    ${PROJECT_SOURCE_DIR}/../src
    ${GST_INCLUDE_DIRS}
)
target_link_libraries(test_mavlink_protocol
    pthread
    ${GST_LIBRARIES}
)
IF(Avahi_FOUND)
    target_link_libraries(test_mavlink_protocol
        Avahi::client
        Avahi::glib
    )
ENDIF(Avahi_FOUND)

IF(Avahi_FOUND)
    add_executable(test_rtsp_udp_stream_discovery
        test_rtsp_udp_stream_discovery.cpp
        ../src/glib_mainloop.cpp
        ../src/log.cpp
        ../src/mainloop.cpp
    )
    target_include_directories(test_rtsp_udp_stream_discovery PUBLIC
        ${PROJECT_SOURCE_DIR}/../modules/mavlink/common
        ${PROJECT_SOURCE_DIR}/../src
        ${GST_INCLUDE_DIRS}
    )
    target_link_libraries(test_rtsp_udp_stream_discovery
        Avahi::client
        Avahi::glib
    )
ENDIF(Avahi_FOUND)

target_link_libraries(test_rtsp_udp_stream_discovery
    pthread
    ${GST_LIBRARIES}
)

