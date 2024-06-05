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
    http_server->start();
}

int main(int argc, char** argv) {
    orange::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
