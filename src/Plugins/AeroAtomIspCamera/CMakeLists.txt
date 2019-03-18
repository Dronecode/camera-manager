file(GLOB SRC_FILES
    CameraDeviceAeroAtomIsp.cpp
    PluginAeroAtomIsp.cpp)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(AeroAtomIspCamera ${SRC_FILES})
