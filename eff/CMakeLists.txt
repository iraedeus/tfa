cmake_minimum_required(VERSION 3.16)
project(GrammarEFFApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку meta-object compiler, UI и ресурсов Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Находим пакеты Qt (скрипт поддерживает как Qt6, так и Qt5)
find_package(Qt6 COMPONENTS Widgets REQUIRED QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_LIBRARIES Qt5::Widgets)
else()
    set(QT_LIBRARIES Qt6::Widgets)
endif()

# Главный исполняемый файл
add_executable(GrammarEFFApp main.cpp)

# Связываем с библиотеками Qt
target_link_libraries(GrammarEFFApp PRIVATE ${QT_LIBRARIES})

