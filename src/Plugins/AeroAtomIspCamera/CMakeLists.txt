include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(AeroAtomIspCamera
    CameraDeviceAeroAtomIsp.cpp
    PluginAeroAtomIsp.cpp
)

