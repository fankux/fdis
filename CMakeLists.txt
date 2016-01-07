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
        ${FMAP}
        common/fbit.h
        common/fbit.c
        common/fqueue.h
        common/fqueue.hpp
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

        rpc.h

        proto/meta.pb.h
        proto/meta.pb.cc
        proto/modelService.pb.h
        proto/modelService.pb.cc
        )
set(THREADPOOL
        threadpool.h
        threadpool.c
        )
set(RPC_CLIENT
        RpcClient.cpp
        InvokeController.cpp
        )
set(RPC_SERVER
        RpcServer.cpp
        )
set(WOKER
        Worker.h
        Worker.cpp
        RpcServer.cpp
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

add_executable(provider ${MODULE_COMMON} ${RPC_SERVER})
set_target_properties(provider PROPERTIES COMPILE_FLAGS "-DDEBUG_RPCSERVER")

add_executable(worker ${MODULE_COMMON} ${WOKER})
set_target_properties(worker PROPERTIES COMPILE_FLAGS "-DDEBUG_WORKER -DLOG_LEVEL=10")

add_executable(client ${MODULE_COMMON} ${RPC_CLIENT})
set_target_properties(client PROPERTIES COMPILE_FLAGS "-DDEBUG_RPCCLIENT -fpermissive")

add_custom_target(distclean rm -rf CMakeFiles cmake_install.cmake CMakeCache.txt Makefile core)
