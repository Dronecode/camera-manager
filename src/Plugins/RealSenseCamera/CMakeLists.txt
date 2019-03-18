file(GLOB SRC_FILES
    CameraDeviceRealSense.cpp
    PluginRealSense.cpp)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(RealSenseCamera ${SRC_FILES})
