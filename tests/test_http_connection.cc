#include "src/http/http_connection.h"
#include "src/iomanager.h"
#include "src/log.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_pool() {
    orange::http::HttpConnectionPool::ptr pool(new orange::http::HttpConnectionPool("www.baidu.com:80", "", 80, 10, 1000 * 30, 20));
    orange::IOManager::GetThis()->addTimer(1000, [pool]() {
        ORANGE_LOG_DEBUG(g_logger) << "===================================";
        auto r = pool->doGet("/", 300);
        ORANGE_LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run() {
    orange::IPAddress::ptr addr = orange::Address::LookupAnyIPAddress("www.baidu.com:80");
    // addr->setPort(80);
    if(!addr) {
        ORANGE_LOG_DEBUG(g_logger) << "Lookup IP Address error...";
        return;
    }
    orange::Socket::ptr sock = orange::Socket::CreateTCPSocket();
    if(!sock) {
        ORANGE_LOG_DEBUG(g_logger) << "CreateTCP Sock error...";
        return;
    }
    bool rt = sock->connect(addr);
    if(!rt) {
        ORANGE_LOG_DEBUG(g_logger) << "sock connect error...";
        return;
    }
    orange::http::HttpConnection::ptr conn(new orange::http::HttpConnection(sock));
    orange::http::HttpRequest::ptr req(new orange::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setPath("/");
    ORANGE_LOG_INFO(g_logger) << "req:" << std::endl
            << *req;
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if(!rsp) {
        ORANGE_LOG_DEBUG(g_logger) << "recv response error...";
        return;
    }
    ORANGE_LOG_INFO(g_logger) << "rsp:" << std::endl
            << *rsp;

    ORANGE_LOG_INFO(g_logger) << "============================================";
    auto r = orange::http::HttpConnection::DoGet("http://www.baidu.com", 300);
    ORANGE_LOG_INFO(g_logger) << "result=" << r->result
            << " response=" << (r->response ? r->response->toString() : "")
            << " error=" << r->error;
}

int main(int argc, char** argv) {
    orange::IOManager iom(2);
    // iom.schedule(run);
    iom.schedule(test_pool);
    return 0;
}
