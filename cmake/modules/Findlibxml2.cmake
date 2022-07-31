message(STATUS "using bundled Findlibxml2.cmake")

find_path(
    LIBXML2_INCLUDE_DIR
    libxml/tree.h
    /usr/include/libxml2
)

find_library(
    LIBXML2_LIB_DIR
    libxml2.so
)

message(STATUS "set LIBXML2_INCLUDE_DIR = ${LIBXML2_INCLUDE_DIR}")
message(STATUS "set LIBXML2_LIB_DIR = ${LIBXML2_LIB_DIR}")