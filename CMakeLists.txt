cmake_minimum_required(VERSION 3.0.0)
# set the project name and version
project(aggregator VERSION 1.0)

# specify the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add compilation flags
set(CMAKE_C_FLAGS "-g -Wall -Werror -fPIC ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-g -Wall -Werror -fPIC ${CMAKE_CXX_FLAGS}")


# application entrance
add_executable(error_enum_string error_enum_string.cpp)
