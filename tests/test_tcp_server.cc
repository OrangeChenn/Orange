#include "tcp_server.h"
#include "log.h"
#include "iomanager.h"
#include "hook.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void run() {
    auto addr = orange::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = orange::UnixAddress::ptr(new orange::UnixAddress("/tmp/unix_chen_addr"));
    std::vector<orange::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);
    std::vector<orange::Address::ptr> fails;
    orange::TcpServer::ptr server(new orange::TcpServer);
    while(!server->bind(addrs, fails)) {
        sleep(2);
    }
    server->start();
}

int main(int argc, char** argv) {
    orange::IOManager iom;
    iom.schedule(run);
    return 0;
}
