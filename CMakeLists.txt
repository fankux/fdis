cmake_minimum_required(VERSION 2.8.4)
project(flog)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m32 -g")
set(SOURCE_DIR ".")
set(SOURCE_FILES common.c
                 fstr.c
                 fmem.c
                 fmap.c
                 flog.c
                 http.c)

include_directories(${SOURCE_DIR})
add_executable(flog ${SOURCE_FILES})