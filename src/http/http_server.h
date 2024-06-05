#pragma once

#include "tcp_server.h"

namespace orange {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false, IOManager* worker = IOManager::GetThis()
                             , IOManager* accept_worker = IOManager::GetThis());

    void handleClient(orange::Socket::ptr client) override;
private:
    bool m_iskeepalive;
};

} // namespace orange
} // namespace http
