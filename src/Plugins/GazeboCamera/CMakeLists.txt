find_package(gazebo REQUIRED)

file(GLOB SRC_FILES
    CameraDeviceGazebo.cpp
    PluginGazebo.cpp)

include_directories(
    ${GAZEBO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(GazeboCamera ${SRC_FILES})
