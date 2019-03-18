include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(RealSenseCamera
    CameraDeviceRealSense.cpp
    PluginRealSense.cpp)

