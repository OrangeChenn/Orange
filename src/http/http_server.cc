#include "http_server.h"
#include "http_session.h"
#include "log.h"

namespace orange {
namespace http {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, IOManager* worker, IOManager* accept_worker) 
    :TcpServer(worker, accept_worker)
    ,m_iskeepalive(keepalive){
}

void HttpServer::handleClient(orange::Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        HttpRequest::ptr req = session->recvResquest();
        if(!req) {
            ORANGE_LOG_WARN(g_logger) << "recv http request fail, "
                    << "errno=" << errno << " strerror=" << strerror(errno)
                    << *client;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() | !m_iskeepalive));
        rsp->setBody("hello chen");
        ORANGE_LOG_INFO(g_logger) << "request:" << std::endl
                << *req;
        ORANGE_LOG_INFO(g_logger) << "response:" << std::endl
                << *rsp;
        session->sendResponse(rsp);
    } while(m_iskeepalive);
    session->close();
}

} // namespace orange
} // namespace http
