//
// Created by fankux on 16-4-5.
//

#ifndef FANKUX_INVOKECONTROLLER_H
#define FANKUX_INVOKECONTROLLER_H

#include "rpc/rpc.h"

namespace fankux {
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

#endif // FANKUX_INVOKECONTROLLER_H
