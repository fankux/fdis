cmake_minimum_required(VERSION 2.8.4)
project(webc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -pthread -fpermissive -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m64 -g -pthread")

set(MODULE_COMMON
        common/common.h
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

        proto/modelService.pb.h
        proto/modelService.pb.cc
        )
set(THREADPOOL
        threadpool.h
        threadpool.c
        )
set(RPC_CONSUMER
        rpcConsumer.cpp
        )
set(RPC_PROVIDER
        rpcProvider.cpp
        rpcProvider.h
        )

set(MODULE_COMMON_INC
        .
        deps/src
        )

set(MODULE_COMMON_LIB
        /usr/local/lib/libprotoc.a
        /usr/local/lib/libprotobuf.a
        )

include_directories(${MODULE_COMMON_INC})
link_libraries(${MODULE_COMMON_LIB})
add_executable(webc ${MODULE_COMMON})
add_executable(threadpool ${MODULE_COMMON} ${THREADPOOL})
add_executable(provider ${MODULE_COMMON} ${RPC_PROVIDER})
add_executable(consumer ${MODULE_COMMON} ${RPC_CONSUMER})