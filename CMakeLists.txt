cmake_minimum_required(VERSION 3.0.0)

# set the project name and version
project(cxxtricks VERSION 1.0)

# specify the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add compilation flags
set(CMAKE_C_FLAGS "-g -Wall -Werror -fPIC ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-g -Wall -Werror -fPIC ${CMAKE_CXX_FLAGS}")

# set cmake module path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules;${CMAKE_MODULE_PATH}")

# find luajit package
find_package(luajit REQUIRED)

# application entrance
add_executable(error_enum_string error_enum_string.cpp)

add_executable(thread_based_lua_vm thread_based_lua_vm.cpp)

add_executable(spsc_queue spsc_queue.cpp)

target_include_directories(
    thread_based_lua_vm
    PUBLIC ${LUAJIT_INCLUDE_DIR}
)

target_include_directories(
    spsc_queue
    PUBLIC $(CMAKE_CURRENT_SOURCE_DIR)
)

target_link_libraries(thread_based_lua_vm pthread ${LUAJIT_LIB_DIR})

target_link_libraries(spsc_queue pthread)
