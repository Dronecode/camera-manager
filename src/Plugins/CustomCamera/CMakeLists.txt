file(GLOB SRC_FILES *.cpp)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

add_library(CustomCamera ${SRC_FILES})