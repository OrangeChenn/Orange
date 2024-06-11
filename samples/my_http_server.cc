#include "http/http_server.h"
#include "log.h"
#include "iomanager.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void run() {
    g_logger->setLevel(orange::LogLevel::INFO);
    orange::Address::ptr addr = orange::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        ORANGE_LOG_INFO(g_logger) << "create addr fail";
        return;
    }
    orange::http::HttpServer::ptr server(new orange::http::HttpServer);
    while(!server->bind(addr)) {
        ORANGE_LOG_INFO(g_logger) << "bind addr fail";
        sleep(2);
    }
    server->start();
}

int main(int argc, char** argv) {
    orange::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
