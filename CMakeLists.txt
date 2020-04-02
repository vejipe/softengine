cmake_minimum_required(VERSION 2.8)

project(softengine)

# https://stackoverflow.com/a/44900762
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
