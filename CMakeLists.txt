cmake_minimum_required(VERSION 2.8.4)
project(flog)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m32 -g")
set(SOURCE_DIR ".")
set(SOURCE_FILES common/common.h
                 common/common.c
                 common/flog.h
                 common/flog.c
                 common/fmem.h
                 common/fmem.c
                 common/fstr.h
                 common/fstr.c
                 common/flist.h
                 common/flist.c
                 common/fmap.h
                 common/fmap.c
                 http.h
                 http.c
                 webc.h
                 webc.c
                 arranger.h
                 arranger.c
                 threadpool.h
                 threadpool.c
                 )

include_directories(${SOURCE_DIR} common)
add_executable(flog ${SOURCE_FILES} common/fqueue.h common/fqueue.c common/fbit.h common/fbit.c)