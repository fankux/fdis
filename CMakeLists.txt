cmake_minimum_required(VERSION 2.8.4)
project(webc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -pthread -fpermissive -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m64 -g -pthread")

set(FMAP
        common/fmap.h
        common/fmap.hpp
        common/fmap.c
        )

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
        common/flist.hpp
        ${FMAP}
        common/fbit.h
        common/fbit.c
        common/fqueue.h
        common/fqueue.hpp
        common/fqueue.c
        common/check.h

        webc.h
        webc.c
        cmd.h
        cmd.c
        net.h
        net.c
        event.h
        event.c

        rpc/rpc.h

        proto/meta.pb.h
        proto/meta.pb.cc
        proto/heartbeat.pb.h
        proto/heartbeat.pb.cc
        proto/ArrangerService.pb.h
        proto/ArrangerService.pb.cc
        )
set(THREADPOOL
        threadpool.h
        threadpool.c
        )

set(WOKER
        rpc/RpcServer.h
        rpc/RpcServer.cpp

        rpc/RpcClient.h
        rpc/RpcClient.cpp
        rpc/InvokeController.cpp

        worker/worker.h
        worker/worker.cpp
        )

set(ARRANGER
        rpc/RpcServer.h
        rpc/RpcServer.cpp

        arranger.h
        arranger.cpp
        ArrangerServer.h
        ArrangerServer.cpp
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

add_executable(fmap ${FMAP} ${MODULE_COMMON})
set_target_properties(fmap PROPERTIES COMPILE_FLAGS "-DDEBUG_FMAP -Wall")

add_executable(webc ${MODULE_COMMON})

add_executable(threadpool ${MODULE_COMMON} ${THREADPOOL})

add_executable(arranger ${MODULE_COMMON} ${ARRANGER})
set_target_properties(arranger PROPERTIES COMPILE_FLAGS "-DDEBUG_ARRANGER -DLOG_LEVEL=10")

add_executable(node ${MODULE_COMMON} ${WOKER})
set_target_properties(node PROPERTIES COMPILE_FLAGS "-DDEBUG_WORKER -DLOG_LEVEL=10")

add_custom_target(distclean rm CMakeFiles cmake_install.cmake CMakeCache.txt Makefile core)
