#include "src/http/http_server.h"
#include "src/log.h"
#include "hook.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void run() {
    orange::http::HttpServer::ptr http_server(new orange::http::HttpServer);
    orange::Address::ptr addr = orange::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!http_server->bind(addr)) {
        sleep(2);
    }
    auto sd = http_server->getServletDispatch();
    sd->addServlet("/orange/xx", [] (orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) {
        response->setBody(request->toString());
        return 0;
    });
    sd->addGlobServlet("/orange/*", [] (orange::http::HttpRequest::ptr request
                    , orange::http::HttpResponse::ptr response
                    , orange::http::HttpSession::ptr session) {
        response->setBody("Glob\r\n" + request->toString());
        return 0;
    });
    http_server->start();
}
std::function<void(orange::http::HttpRequest::ptr, orange::http::HttpResponse::ptr, orange::http::HttpSession::ptr)> callback =
    [](orange::http::HttpRequest::ptr request, orange::http::HttpResponse::ptr response, orange::http::HttpSession::ptr session) {
        // lambda 函数的实现
    };

int main(int argc, char** argv) {
    orange::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
