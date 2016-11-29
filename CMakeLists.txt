cmake_minimum_required(VERSION 2.8.4)
project(webc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -fpermissive -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m64 -g -pthread")

set(FMAP
        src/common/fmap.h
        src/common/fmap.hpp
        src/common/fmap.c
        )
set(FSTR
        src/common/fstr.h
        src/common/fstr.hpp
        src/common/fstr.c
        )

set(PROTO
        proto/gen.sh
        proto/meta.proto
        proto/meta.pb.h
        proto/meta.pb.cc
        proto/heartbeat.proto
        proto/heartbeat.pb.h
        proto/heartbeat.pb.cc
        proto/ArrangerService.proto
        proto/ArrangerService.pb.h
        proto/ArrangerService.pb.cc
        proto/WorkerService.proto
        proto/WorkerService.pb.h
        proto/WorkerService.pb.cc
        )

set(FS
        src/fs/file_access.h
        )

set(MODULE_COMMON
        src/common/common.h
        src/common/common.c
        src/common/flog.h
        src/common/flog.c
        src/common/fmem.h
        src/common/fmem.c
        ${FSTR}
        src/common/flist.h
        src/common/flist.c
        src/common/flist.hpp
        ${FMAP}
        src/common/fbit.h
        src/common/fbit.c
        src/common/fqueue.h
        src/common/fqueue.hpp
        src/common/fqueue.c
        src/common/check.h

        src/webc.h
        src/webc.c
        src/cmd.h
        src/cmd.c
        src/net.h
        src/net.c
        src/event.h
        src/event.c
        src/config.h
        src/config.c

        src/rpc/rpc.h

        src/threadpool.hpp
        src/threadpool.h
        src/threadpool.c

        ${PROTO}
        ${FS}
        )

set(WOKER
        src/rpc/RpcServer.h
        src/rpc/RpcServer.cpp
        src/rpc/RpcClient.h
        src/rpc/RpcClient.cpp
        src/rpc/InvokeController.cpp

        src/chunkServer.h
        src/chunkServer.cpp

        src/worker/worker.h
        src/worker/worker.cpp
        )

set(ARRANGER
        src/rpc/RpcServer.h
        src/rpc/RpcServer.cpp

        src/arranger.h
        src/arranger.cpp
        src/ArrangerServer.h
        src/ArrangerServer.cpp
        src/ArrangerConf.cpp
        )

set(CLIENT
        src/client.cpp
        )

set(MODULE_COMMON_INC
        .
        src
        src/deps/src
        src/se
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

add_executable(worker ${MODULE_COMMON} ${WOKER})
set_target_properties(worker PROPERTIES COMPILE_FLAGS "-DDEBUG_WORKER -DLOG_LEVEL=10")

add_executable(client ${MODULE_COMMON} ${CLIENT})

add_custom_target(distclean rm CMakeFiles cmake_install.cmake CMakeCache.txt Makefile core)
