//
// Created by fankux on 16-4-5.
//

#ifndef FDIS_INVOKECONTROLLER_H
#define FDIS_INVOKECONTROLLER_H

#include "rpc/rpc.h"

namespace fdis {
class InvokeController : public google::protobuf::RpcController {

public:
    inline InvokeController() : _success(true), _errmsg("") {}

    void Reset();

    bool Failed() const;

    std::string ErrorText() const;

    void StartCancel();

    void SetFailed(const std::string& reason);

    bool IsCanceled() const;

    void NotifyOnCancel(google::protobuf::Closure* callback);

private:
    bool _success;
    std::string _errmsg;
};

}

#endif // FDIS_INVOKECONTROLLER_H
