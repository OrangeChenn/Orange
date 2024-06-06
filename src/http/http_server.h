#pragma once

#include "servlet.h"
#include "tcp_server.h"

namespace orange {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis()
                             , IOManager* accept_worker = IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() { return m_dispatch; }

    void handleClient(orange::Socket::ptr client) override;
private:
    ServletDispatch::ptr m_dispatch;
    bool m_iskeepalive;
};

} // namespace orange
} // namespace http
