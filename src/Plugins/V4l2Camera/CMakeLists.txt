include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(V4l2Camera
     CameraDeviceV4l2.cpp
     PluginV4l2.cpp)

