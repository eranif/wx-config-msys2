cmake_minimum_required(VERSION 3.0)

message(STATUS "CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
add_library(utilslib STATIC "${CMAKE_CURRENT_LIST_DIR}/utils.cpp")
add_executable(wx-config-msys2 "${CMAKE_CURRENT_LIST_DIR}/wx-config-msys2.cpp")
add_executable(wx-config "${CMAKE_CURRENT_LIST_DIR}/wx-config.cpp")

set_property(TARGET wx-config-msys2 PROPERTY CXX_STANDARD 20)
set_property(TARGET wx-config PROPERTY CXX_STANDARD 20)
set_property(TARGET utilslib PROPERTY CXX_STANDARD 20)

target_link_libraries(wx-config utilslib)
target_link_libraries(wx-config-msys2 utilslib)
