message(STATUS "using bundled Findluajit.cmake")

find_path(
    LUAJIT_INCLUDE_DIR luajit.h
    PATHS /usr/include
    PATH_SUFFIXES luajit-2.0 luajit-2.1
)

find_library(
    LUAJIT_LIB_DIR
    NAMES libluajit-5.1.so
)

message(STATUS "set LUAJIT_INCLUDE_DIR = ${LUAJIT_INCLUDE_DIR}")
message(STATUS "set LUAJIT_LIB_DIR = ${LUAJIT_LIB_DIR}")