cmake_minimum_required(VERSION 3.16)
project(PostmanProblem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим библиотеку Qt6 для создания GUI
find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(PostmanGUI main.cpp)
target_link_libraries(PostmanGUI PRIVATE Qt6::Widgets)
