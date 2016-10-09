cmake_minimum_required(VERSION 2.8.11)

project(testproject)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Quick)

qt5_add_resources(resources/resources.qrc)

add_executable(radiance WIN32 src/main.cpp resources/resources.qrc)

target_link_libraries(radiance Qt5::Widgets Qt5::Quick)