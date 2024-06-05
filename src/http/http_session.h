#pragma once 

#include "http.h"
#include "stream/socket_stream.h"

namespace orange {
namespace http {

class HttpSession : public orange::SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(orange::Socket::ptr socket, bool owner = true);

    HttpRequest::ptr recvResquest();
    int sendResponse(HttpResponse::ptr rsp);
};

} // namespace http
} // namespace orange
