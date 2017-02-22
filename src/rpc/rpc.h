#ifndef FDIS_RPC_H
#define FDIS_RPC_H

#include <stdint.h>
#include "google/protobuf/service.h"

#include "common/fmap.hpp"
#include "common/flist.hpp"
#include "event.h"

namespace fdis {

typedef int64_t procedure_id_t;
typedef google::protobuf::Closure Closure;
typedef google::protobuf::Service Service;
typedef google::protobuf::MethodDescriptor Method;
typedef google::protobuf::Message Message;

#define build_procedure_id(__service_id__, __method_id__) \
    (((__service_id__) << (sizeof(__service_id__) * 8)) | (__method_id__))
}

struct ServerConf {
    uint16_t listen_port;
    size_t recv_buf_size;
};

#endif // FDIS_RPC_H
