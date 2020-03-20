cmake_minimum_required(VERSION 2.8.4)
project(webc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m64 -pthread -fpermissive -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -m64 -g -pthread")

set(FSTR
        src/common/fstr.h
        src/common/fstr.c
        src/common/fstr.hpp
        )
set(LIST
        ${FSTR}
        src/common/flist.h
        src/common/flist.c
        src/common/flist.hpp

        src/common/fmap.h
        src/common/fmap.c
        src/common/sortlist.hpp
        )
set(FMAP
        src/common/fmap.h
        src/common/fmap.c
        src/common/fmap.hpp
        )

set(COMMON
        src/common/common.h
        src/common/common.c
        src/common/flog.h
        src/common/flog.c
        src/common/fmem.h
        src/common/fmem.c
        ${FSTR}
        ${LIST}
        ${FMAP}
        src/common/fbit.h
        src/common/fbit.c
        src/common/fqueue.h
        src/common/fqueue.hpp
        src/common/fqueue.c
        src/common/check.h

        src/common/bytes.h
        src/common/bytes.cpp
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

set(WOKER
        ${COMMON}

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
        ${COMMON}

        src/rpc/RpcServer.h
        src/rpc/RpcServer.cpp

        src/arranger.h
        src/arranger.cpp
        src/ArrangerServer.h
        src/ArrangerServer.cpp
        src/ArrangerConf.cpp
        src/ArrangerConf.h
        )

set(CLIENT
        ${COMMON}

        src/client.cpp
        )

set(THREADPOOL
        src/threadpool.h
        src/threadpool.c
        src/threadpool.hpp
        )

set(FDIS
        ${COMMON}

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

        ${PROTO}
        ${FS}
        )

set(FDIS_INC
        .
        src
        src/deps/src
        src/se
        )

set(FDIS_LIB
        /usr/local/lib/libprotoc.a
        /usr/local/lib/libprotobuf.a
        )

include_directories(${FDIS_INC})
link_libraries(${MFDIS_LIB})

add_executable(threadpool ${THREADPOOL})

add_executable(arranger ${ARRANGER})
set_target_properties(arranger PROPERTIES COMPILE_FLAGS "-DDEBUG_ARRANGER -DLOG_LEVEL=10")

add_executable(worker ${WOKER})
set_target_properties(worker PROPERTIES COMPILE_FLAGS "-DDEBUG_WORKER -DLOG_LEVEL=10")

add_executable(client ${CLIENT})

add_executable(fdis ${FDIS})

add_executable(fmap_test test/fmap_test.cpp ${FMAP})
set_target_properties(fmap_test PROPERTIES COMPILE_FLAGS "-DDEBUG_FMAP -Wall")

add_executable(list_test test/list_test.cpp ${LIST})
set_target_properties(list_test PROPERTIES COMPILE_FLAGS "-DDEBUG_FLIST -Wall")

add_custom_target(distclean rm CMakeFiles cmake_install.cmake CMakeCache.txt Makefile core)
