cmake_minimum_required(VERSION 3.16)
project(EasyMacro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.0/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(EasyMacro WIN32
        main.cpp
        src/MacroEvent.h
        src/MacroEvent.cpp
        src/MacroRecorder.h
        src/MacroRecorder.cpp
        src/MacroPlayer.h
        src/MacroPlayer.cpp
        src/Settings.h
        src/Settings.cpp
        src/SettingsDialog.h
        src/SettingsDialog.cpp
        src/LicenseDialog.h
        src/LicenseDialog.cpp
        src/MainWindow.h
        src/MainWindow.cpp
)

target_include_directories(EasyMacro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(EasyMacro PRIVATE
        Qt6::Widgets
        Qt6::Core
        user32
)
