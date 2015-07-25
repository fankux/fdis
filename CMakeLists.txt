cmake_minimum_required(VERSION 2.8.4)
project(webc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m32 -g -pthread")
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
                 common/fbit.h
                 common/fbit.c
                 common/fqueue.h
                 common/fqueue.c
                 common/check.h
                 webc.h
                 webc.c
                 arranger.h
                 arranger.c
                 cmd.h
                 cmd.c
                 net.h
                 net.c
                 event.h
                 event.c
                 threadpool.h
                 threadpool.c
                 )

include_directories(${SOURCE_DIR})
add_executable(webc ${SOURCE_FILES})