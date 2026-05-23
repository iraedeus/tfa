cmake_minimum_required(VERSION 3.16)
project(CityDistributor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

add_executable(CityDistributor
    graph.h
    graph.cpp
    mainwindow.h
    mainwindow.cpp
)

target_link_libraries(CityDistributor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
