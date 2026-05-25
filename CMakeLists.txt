cmake_minimum_required(VERSION 3.16)
project(EasyMacro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.0/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(EasyMacro WIN32 main.cpp)


target_link_libraries(EasyMacro PRIVATE
        Qt6::Widgets
        Qt6::Core
)

set_target_properties(EasyMacro PROPERTIES
        AUTOUIC ON
        AUTOMOC ON
)