cmake_minimum_required(VERSION 3.16)
project(brush-synth)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "E:/qt/6.10.2/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/MainWindow.h
    include/TextureGenerator.h
    include/PreviewWidget.h
)

if(WIN32)
    add_executable(brush-synth ${SOURCES} ${HEADERS})
else()
    add_executable(brush-synth ${SOURCES} ${HEADERS})
endif()

target_link_libraries(brush-synth PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)
