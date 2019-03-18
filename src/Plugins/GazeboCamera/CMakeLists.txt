find_package(gazebo REQUIRED)

include_directories(
    ${GAZEBO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(GazeboCamera
    CameraDeviceGazebo.cpp
    PluginGazebo.cpp)

