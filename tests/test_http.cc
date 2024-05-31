#include "http/http.h"
#include "log.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_request() {
    orange::http::HttpRequest::ptr req(new orange::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello chen");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    orange::http::HttpResponse::ptr res(new orange::http::HttpResponse);
    res->setHeader("X-X", "chen");
    res->setBody("hello chen");
    res->setStatus((orange::http::HttpStatus)400);
    res->setClose(false);
    res->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
