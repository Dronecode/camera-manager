include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(CustomCamera
    CameraDeviceCustom.cpp
    PluginCustom.cpp)
