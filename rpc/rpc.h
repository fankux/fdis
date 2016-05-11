#ifndef FANKUX_RPC_H
#define FANKUX_RPC_H

#include <stdint.h>
#include "google/protobuf/service.h"

#include "common/fmap.hpp"
#include "common/flist.hpp"
#include "event.h"

namespace fankux {

typedef int64_t procedure_id_t;
typedef google::protobuf::Closure Closure;
typedef google::protobuf::Service Service;
typedef google::protobuf::MethodDescriptor Method;
typedef google::protobuf::Message Message;

#define build_procedure_id(__service_id__, __method_id__) \
    (((__service_id__) << (sizeof(__service_id__) * 8)) | (__method_id__))
}

#endif // FANKUX_RPC_H
