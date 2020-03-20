#include "rpc/InvokeController.h"

namespace fdis {
void InvokeController::Reset() {
    _success = true;
    _errmsg.clear();
}

bool InvokeController::Failed() const {
    return !_success;
}

std::string InvokeController::ErrorText() const {
    return _errmsg;
}

void InvokeController::StartCancel() {
    // not impl
}

void InvokeController::SetFailed(const std::string& reason) {
    _success = false;
    _errmsg = reason;
}

bool InvokeController::IsCanceled() const {
    // not impl
    return false;
}

void InvokeController::NotifyOnCancel(google::protobuf::Closure* callback) {
    if (callback != NULL) {
        callback->Run();
    }
}
}